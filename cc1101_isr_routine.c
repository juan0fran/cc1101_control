#include "cc1101_utils.h"
#include "cc1101_config.h"

#define TX_FIFO_REFILL 60 // With the default FIFO thresholds selected this is the number of bytes to refill the Tx FIFO
#define RX_FIFO_UNLOAD 59 // With the default FIFO thresholds selected this is the number of bytes to unload from the Rx FIFO

extern radio_int_data_t *p_radio_int_data;

// === Interupt handlers ==========================================================================

// ------------------------------------------------------------------------------------------------
// Processes packets up to 255 bytes
void gdo0_isr(void)
// ------------------------------------------------------------------------------------------------
{
    uint8_t x_byte, *p_byte, int_line, rssi_dec, crc_lqi;
    int i;
    if (p_radio_int_data == NULL){
    	return;
    }
    int_line = CC11xx_PIN(GDO0_pin); // Sense interrupt line to determine if it was a raising or falling edge

    if (p_radio_int_data->mode == RADIOMODE_RX)
    {
        if (int_line)
        {         
            p_radio_int_data->byte_index = 0;
            p_radio_int_data->rx_count = p_radio_int_data->packet_length + 2;
            p_radio_int_data->bytes_remaining = p_radio_int_data->rx_count;
            p_radio_int_data->packet_receive = 1; // reception is in progress
        }
        else
        {
            if (p_radio_int_data->packet_receive) // packet has been received
            {
                CC_SPIReadBurstReg(p_radio_int_data->spi_parms, CC11xx_RXFIFO, &p_byte, p_radio_int_data->bytes_remaining);
                memcpy((uint8_t *) &(p_radio_int_data->rx_buf[p_radio_int_data->byte_index]), p_byte, p_radio_int_data->bytes_remaining);
                p_radio_int_data->byte_index += p_radio_int_data->bytes_remaining;
                p_radio_int_data->bytes_remaining = 0;

                p_radio_int_data->mode = RADIOMODE_NONE;
                p_radio_int_data->packet_receive = 0; // reception is done
                p_radio_int_data->packet_rx_count++;
                /* Just send the packet into the queue, gosh this is so easy --> fucking ISR */
                radio_init_rx(p_radio_int_data->spi_parms, p_radio_int_data->radio_parms);
                radio_turn_rx(p_radio_int_data->spi_parms);
            }
        }    
    }    
    else if (p_radio_int_data->mode == RADIOMODE_TX)
    {
        if (int_line)
        {
            p_radio_int_data->packet_send = 1; // Assert packet transmission after sync has been sent
        }
        else
        {
            if (p_radio_int_data->packet_send) // packet has been sent
            {
                p_radio_int_data->mode = RADIOMODE_NONE;
                p_radio_int_data->packet_send = 0; // De-assert packet transmission after packet has been sent
                p_radio_int_data->packet_tx_count++;

                if ((p_radio_int_data->bytes_remaining))
                {
                    radio_flush_fifos(p_radio_int_data->spi_parms);
                }
                /* Just put again into RX, this is a fucking holy easy shit */
                radio_init_rx(p_radio_int_data->spi_parms, p_radio_int_data->radio_parms);
                radio_turn_rx(p_radio_int_data->spi_parms);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Processes packets that do not fit in Rx or Tx FIFOs and 255 bytes long maximum
// FIFO threshold interrupt handler 
void gdo2_isr(void)
// ------------------------------------------------------------------------------------------------
{
    uint8_t i, int_line, bytes_to_send, x_byte, *p_byte;
    if (p_radio_int_data == NULL){
    	return;
    }
    int_line = CC11xx_PIN(GDO2_pin); // Sense interrupt line to determine if it was a raising or falling edge

    if ((p_radio_int_data->mode == RADIOMODE_RX) && (int_line)) // Filling of Rx FIFO - Read next 59 bytes
    {
        if (p_radio_int_data->packet_receive) // if reception has started
        {
            CC_SPIReadBurstReg(p_radio_int_data->spi_parms, CC11xx_RXFIFO, &p_byte, RX_FIFO_UNLOAD);
            memcpy((uint8_t *) &(p_radio_int_data->rx_buf[p_radio_int_data->byte_index]), p_byte, RX_FIFO_UNLOAD);
            p_radio_int_data->byte_index += RX_FIFO_UNLOAD;
            p_radio_int_data->bytes_remaining -= RX_FIFO_UNLOAD;            
        }
    }
    else if ((p_radio_int_data->mode == RADIOMODE_TX) && (!int_line)) // Depletion of Tx FIFO - Write at most next TX_FIFO_REFILL bytes
    {
        if ((p_radio_int_data->packet_send) && (p_radio_int_data->bytes_remaining > 0)) // bytes left to send
        {
            if (p_radio_int_data->bytes_remaining < TX_FIFO_REFILL)
            {
                bytes_to_send = p_radio_int_data->bytes_remaining;
            }
            else
            {
                bytes_to_send = TX_FIFO_REFILL;
            }

            CC_SPIWriteBurstReg(p_radio_int_data->spi_parms, CC11xx_TXFIFO, (uint8_t *) &(p_radio_int_data->tx_buf[p_radio_int_data->byte_index]), bytes_to_send);
            p_radio_int_data->byte_index += bytes_to_send;
            p_radio_int_data->bytes_remaining -= bytes_to_send;
        }
    }
}






