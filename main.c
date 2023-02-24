#include <stdint.h>

#include "stm32f4xx.h"

int putchar(int c) {
  // if(c=='\r') putchar('\n');
  while (!(USART1->SR & USART_SR_TXE)) {
  };               // wait until we are able to transmit
  USART1->DR = c;  // transmit the character
  return c;
}

void print_string(const char* s) {
  while (*s) putchar(*s++);
}

int getchar() {
  while (!(USART1->SR & USART_SR_RXNE)) {
  };                  // wait until something received
  return USART1->DR;  // find out what it is
}

void alertCrash() {
  print_string("Crashed, restart required\n");
  while (1)
    ;
}

int strlen(const char* str) {
  const char* end = str;
  while (*end != 0) {
    end++;
  }
  return end - str;
}

void greet() {
  print_string("STM32F4 rootshell\n");
  print_string("Commands: \n");
  print_string("  \"H\" this message.\n");
  print_string(
      "  \"R aabbccdd 000000FF\" read FF bytes from address aabbccdd. Both "
      "arguments must be zero-padded to 8 characters.\n");
}

const uint8_t hexMap[] = "0123456789ABCDEF";

void printByteHex(uint8_t b) {
  putchar(hexMap[b >> 4]);
  putchar(hexMap[b & 0x0F]);
}

void printWordHexBigEndiand(uint32_t word) {
  printByteHex((word & 0xFF000000) >> 24);
  printByteHex((word & 0x00FF0000) >> 16);
  printByteHex((word & 0x0000FF00) >> 8);
  printByteHex((word & 0x000000FF));
}

void printWordHexLittleEndian(uint32_t word) {
  printByteHex((word & 0x000000FF));
  printByteHex((word & 0x0000FF00) >> 8);
  printByteHex((word & 0x00FF0000) >> 16);
  printByteHex((word & 0xFF000000) >> 24);
}

int parseHexToInt(const char* line, uint32_t* int_out) {
  uint32_t num = 0;
  for (int i = 0; i < 8; i++) {
    const char c = line[i];
    int digit;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    } else {
      print_string("ERROR: unexpected character while parsing hex string: 0x");
      printByteHex(c);
      putchar('\n');
      return 1;
    }
    num = digit | (num << 4);
  }
  if (line[8] != 0 && line[8] != ' ') {
    print_string("ERROR: hex string too long, got char 0x");
    printByteHex(line[8]);
    putchar('\n');
    return 1;
  }
  *int_out = num;
  return 0;
}

void processReadCommand(const char* line) {
  const int len = strlen(line);
  int pos = 1;
  while (pos < len && line[pos] == ' ') {
    pos++;
  }
  uint32_t address;
  if (parseHexToInt(line + pos, &address)) {
    return;
  }
  pos += 8;
  while (pos < len && line[pos] == ' ') {
    pos++;
  }
  uint32_t read_size;
  if (parseHexToInt(line + pos, &read_size)) {
    return;
  }
  print_string("Reading 0x");
  printWordHexBigEndiand(read_size);
  print_string(" bytes from 0x");
  printWordHexBigEndiand(address);
  putchar('\n');

  const uint32_t* last_word = (uint32_t*)(address + read_size);
  const uint32_t* read_address = (uint32_t*)address;
  int word_no = 0;
  while (read_address < last_word) {
    if ((word_no % 8) == 0) {
      putchar('\n');
      printWordHexBigEndiand((uint32_t)read_address);
      print_string(": ");
    }
    printWordHexLittleEndian(*read_address);
    read_address++;
    word_no++;
  }
}

void parseLine(const char* line) {
  switch (line[0]) {
    case 'r':
    case 'R':
      processReadCommand(line);
      break;
    case 'h':
    case 'H':
      greet();
      break;
  }
}

int main(void) {
  // Set pins PA9 (TX) and PA10 (RX) for serial communication
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // enable RCC for port A
  GPIOA->MODER |=
      (GPIO_MODER_MODER9_1 |
       GPIO_MODER_MODER10_1);  // PA9/10 alternate functions for USART1 TX/RX
  GPIOA->AFR[1] |= ((7 << GPIO_AFRH_AFSEL9_Pos) | (7 << GPIO_AFRH_AFSEL10_Pos));

  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // enable RCC for USART1

  // Set the baud rate, which requires a mantissa and fraction
  const uint32_t baud_rate = 256000;
  SystemCoreClockUpdate();
  uint16_t uartdiv = SystemCoreClock / baud_rate;
  USART1->BRR = (((uartdiv / 16) << USART_BRR_DIV_Mantissa_Pos) |
                 ((uartdiv % 16) << USART_BRR_DIV_Fraction_Pos));

  // Mpw enable the USART peripheral
  USART1->CR1 |= USART_CR1_RE    // enable receive
                 | USART_CR1_TE  // enable transmit
                 | USART_CR1_UE  // enable usart
      ;
  greet();
  char last_line[512];
  int last_line_len = 0;
  while (1) {
    int c = getchar();
    putchar(c);
    if (c == '\r') {
      putchar('\n');
    }
    if (c == '\r' || c == '\n') {
      last_line[last_line_len++] = 0;
      parseLine(last_line);
      last_line_len = 0;
    } else {
      last_line[last_line_len++] = c;
    }
  }
}
