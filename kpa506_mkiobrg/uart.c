#include "opora.h"
#include "gdef.h"


char tx_buf[BUF_SIZE];
int  tx_idx = 0;
int  tx_wr_idx = 0;
  
char rx_buf[BUF_SIZE];
int  rx_idx = 0;
int  rx_rd_idx = 0;

static inline int uart_bsz(void)
{
	return (rx_idx - rx_rd_idx)&(BUF_SIZE-1);
}

void uart_init(void)
{
	RST_CLK->PER_CLOCK |= (1 << 7);								// enable clock UART2
	RST_CLK->UART_CLOCK |= (1 << 25);	
		
	// UART_CLK = 96MHz
	// rate = 1000 k div = 96000/16/1000 = 6.0
	// rate = 500 k div = 96000/16/500 = 12.0
	// rate = 921.6 k div = 96000/16/921.6 = 6.5104
	// rate = 115200 k div = 96000/16/115.2 = 52.083
	
	//UART2->IBRD = 6;											// 6
	//UART2->FBRD = 33;											// round(0.5104*2^6) = 33
	
	UART2->IBRD = 12;
	UART2->FBRD = 0;
	
	//UART2->IBRD = 65;											// 65
	//UART2->FBRD = 7;											// round(0.1042*2^6) = 7	

	UART2->IFLS &= ~(UART_IFLS_RXIFLSEL_MASK | UART_IFLS_TXIFLSEL_MASK);
	UART2->IFLS |= (4 << UART_IFLS_RXIFLSEL_OFFS) | (4 << UART_IFLS_TXIFLSEL_OFFS);  // threshold for FIFO is 7/8
	//UART2->LCR_H |= UART_LCR_H_FEN;								// enable FIFO
	UART2->LCR_H |= 3 << UART_LCR_H_WLEN_OFFS;					// word length is 8 bit
	UART2->CR |= (UART_CR_RXE | UART_CR_TXE | UART_CR_UARTEN); 	// enable uart 
	
	// config uart irq
	//UART2->IMSC |= (UART_IMSC_RXIM | UART_IMSC_TXIM);
	UART2->IMSC |= (UART_IMSC_RXIM | UART_IMSC_BEIM);
	NVIC_EnableIRQ(UART2_IRQn);
}

void uart_putch(char ch)
{
	while( UART2->FR & UART_FR_TXFF );	//wait until Tx FIFO full
	UART2->DR = ch;
}

int uart_read(char *pb, int nb)
{
        int i;

        if(rx_rd_idx == rx_idx) return 0;

        for( i = 0; i < nb; i++ ){
                pb[i] = rx_buf[rx_rd_idx];
                rx_rd_idx = (rx_rd_idx+1)&(BUF_SIZE-1);
                if(rx_rd_idx == rx_idx) {
                        i++;
                        break;
                }
        }
        
        // flow control
		int bsz = uart_bsz();				
		if(bsz < BUF_TH_UP) PORTE->RXTX &= ~(1 << 7); // enable flow
        
        return i;
}

int uart_send(char *pb, int nb)
{
	int i;

	//NVIC_DisableIRQ(UART2_IRQn);
	if(tx_wr_idx == tx_idx) {

	}

	for( i = 0; i < nb; i++ ){
			tx_buf[tx_wr_idx] = pb[i];
			tx_wr_idx = (tx_wr_idx+1)&(BUF_SIZE-1);
			if(tx_wr_idx == tx_idx) {
					i++;
					break;
			}
	}

	if((UART2->FR & UART_FR_BUSY) == 0){
		// send first byte
		UART2->DR = tx_buf[tx_idx];
		tx_idx = (tx_idx+1)&(BUF_SIZE-1);
	}

	UART2->IMSC |= UART_IMSC_TXIM;  // Transmit data register empty interrupt enable
	//NVIC_EnableIRQ(UART2_IRQn);
	return i;
}

void UART2_Handler(void)
{

	if(UART2->MIS & UART_MIS_TXMIS)
	{
		if(tx_idx == tx_wr_idx)
		{
			// no data for transfer - interrupt disable
			UART2->IMSC &= ~UART_IMSC_TXIM;
		}
		else
		{
			// transfer next byte from the buffer
			UART2->DR = tx_buf[tx_idx];
			tx_idx = (tx_idx+1)&(BUF_SIZE-1);
		}
	}

	if(UART2->MIS & UART_MIS_RXMIS)
	{       
		rx_buf[rx_idx] = UART2->DR;
		rx_idx = (rx_idx+1)&(BUF_SIZE-1);
		
		// flow control
		int bsz = uart_bsz();				
		if(bsz > BUF_TH_DOWN)	PORTE->RXTX |= (1 << 7); // disable flow

	}        	
	
	if(UART2->MIS & UART_MIS_BEMIS)
	{
		UART2->ICR |= UART_ICR_BEIC;
		rx_idx = 0;
		rx_rd_idx = 0;
		PORTE->RXTX &= ~(1 << 7); // enable flow
	}
}
