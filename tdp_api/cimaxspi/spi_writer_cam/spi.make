all:
	arm-marvell-linux-gnueabi-gcc -Wall spi_writer.c -o spi_writer
clean:
	rm spi_writer
