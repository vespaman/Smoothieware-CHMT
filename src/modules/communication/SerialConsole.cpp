/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/
#include <string>
#include <stdarg.h>
using std::string;
#include "libs/Module.h"
#include "libs/Kernel.h"
#include "libs/nuts_bolts.h"
#include "SerialConsole.h"
#include "libs/RingBuffer.h"
#include "libs/SerialMessage.h"
#include "libs/StreamOutput.h"
#include "libs/StreamOutputPool.h"
#include "libs/gpio.h"
#include "port_api.h"

GPIO rts_signal = GPIO(PA_12);

#define RX_BUFF_SIZE 256
unsigned char rx_buff[RX_BUFF_SIZE];


// Serial reading module
// Treats every received line as a command and passes it ( via event call ) to the command dispatcher.
// The command dispatcher will then ask other modules if they can do something with it
SerialConsole::SerialConsole( PinName rx_pin, PinName tx_pin, PinName rts_pin, PinName cts_pin, int baud_rate ){
    this->serial = new mbed::Serial( rx_pin, tx_pin, rts_pin, cts_pin );
    this->serial->baud(baud_rate);

    if ( rts_pin )
    {
        flow_control = true;
        rts_signal = 0;
    }
    else
        flow_control = false;

    rts_signal_is_set = false;
}

// Called when the module has just been loaded
void SerialConsole::on_module_loaded() {
    this->serial->attach(this, &SerialConsole::on_line_idle, mbed::Serial::RxIdleIrq);
    this->serial->attach(this, &SerialConsole::on_buffer_half_full, mbed::Serial::DmaHFIrq);
    this->serial->attach(this, &SerialConsole::on_buffer_full, mbed::Serial::DmaTCIrq);
    this->serial->dma_init( rx_buff, RX_BUFF_SIZE );
    query_flag= false;
    halt_flag= false;

    // We only call the command dispatcher in the main loop, nowhere else
    this->register_for_event(ON_MAIN_LOOP);
    this->register_for_event(ON_IDLE);

    // Add to the pack of streams kernel can call to, for example for broadcasting
    THEKERNEL->streams->append_stream(this);
}



void SerialConsole::manage_buffer( void )
{
    unsigned char *p;
    int free_space;
    
    bool done = false;
    static unsigned char *tail = rx_buff;
    int remain_until_buf_top = this->serial->get_dma_buffer_index();
    unsigned char *buf_start = rx_buff;
    int buf_len = RX_BUFF_SIZE;
    unsigned char *buf_end = buf_start + buf_len-1;
    unsigned char *head = 1+buf_end - remain_until_buf_top;

    
    // Check if other end may continue to send while we deal with available..
    if (head >= tail)
        free_space = (remain_until_buf_top-1) + (tail-buf_start);
    else
        free_space = tail-head;

    if (free_space <= RX_BUFF_SIZE/2 ) // Ask other end to relax a bit while we deal with this interrupt!
    {
        if (flow_control)
        {
            rts_signal_is_set = true;
            rts_signal = 1; 
        }
    }
    
    do
    {
        p = tail;
        
        do
        {
            if( p == head )
            {
                done = true;
                break;
            }

            if ( (this->buffer.capacity())-this->buffer.size() > 0 )
            {
                
                if( *p == '?') {
                    query_flag= true;
                    continue;
                }
                if( *p == 'X'-'A'+1) { // ^X
                    halt_flag= true;
                    continue;
                }
                if( *p == '\r' )
                    *p = '\n';

                this->buffer.push_back(*p);
            }
            else // no space in secondary buffer, so wait for application to consume!
            {    // (application needs to release rts!)
                if (flow_control)
                {
                    rts_signal_is_set = true;
                    rts_signal = 1;
                }
                done = true;
                break;
            }

            p++;
            if( p > buf_end )
                p = buf_start;

                
        } while ( !done );
    

        // update tail for next time
        tail = p; 

    } while ( !done ); // See if there's more complete messages in buffer


    if (rts_signal_is_set) // Did we showel enough data to secondary buffer?
    {
        if (head >= tail)
            free_space = (remain_until_buf_top-1) + (tail-buf_start);
        else
            free_space = tail-head;

        if (free_space > RX_BUFF_SIZE/2 ) 
        {
            rts_signal_is_set = false;
            rts_signal = 0; 
        }
    }


}


void SerialConsole::on_idle(void * argument)
{
    if(query_flag) {
        query_flag= false;
        puts(THEKERNEL->get_query_string().c_str());
    }
    if(halt_flag) {
        halt_flag= false;
        THEKERNEL->call_event(ON_HALT, nullptr);
        if(THEKERNEL->is_grbl_mode()) {
            puts("ALARM: Abort during cycle\r\n");
        } else {
            puts("HALTED, M999 or $X to exit HALT state\r\n");
        }
    }
}

void SerialConsole::on_main_loop(void * argument){

    int len = this->has_char('\n');
    
    if( len ){
        string received;
        received.reserve(len+1);
        while(1){
           char c;
           this->buffer.pop_front(c);
           if( c == '\n' ){
                struct SerialMessage message;
                message.message = received;
                message.stream = this;
                THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, &message );
                break;
            }else{
                received += c;
            }
        }
        
        if ( rts_signal_is_set )
        {
            if ( (this->buffer.capacity())-this->buffer.size() > 128 ) // If we have stopped other end, only start when we have consumed 
            {                                                          // a good amount in order to never overflow the dma buffer.
                rts_signal_is_set = false;                             
                rts_signal = 0;
            }
        }
    }
    
}

int SerialConsole::puts(const char* s)
{
    this->serial->send_string(s);
    return 0;
}

// Does the queue have a given char ?
int SerialConsole::has_char(char letter){
    int msg_len = 0;
    int index = this->buffer.tail;
    while( index != this->buffer.head ){
        msg_len++;
        if( this->buffer.buffer[index] == letter ){
            return msg_len;
        }
        index = this->buffer.next_block_index(index);
    }
    
    return 0;
}
