
p44_ws2812
==========

[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=luz&url=https://github.com/plan44/ws2812_spi_sparkcore&title= ws2812_spi_sparkcore&language=&tags=github&category=software)

This is a library to drive WS2812 based RGB LED chains.

There are various working approaches to this. The speciality of this particular library is that it uses the SPI hardware to generate the timing.

In the current state, there's no immediate benefit in doing this, because the overall timing is still so tight that IRQs must be blocked (like in all other bit banging approaches). However, SPI on the spark core can also be fed with data via direct memory access (DMA), which runs full speed and without blocking the CPU.

So the goal of this library is to get it work with DMA and thus without needing to block the IRQs any more. Any help from people with experience doing DMA on the spark core is highly welcome.

The current status is that it does pretty much the same as the other libraries for WS2812 on the spark core, only maybe with fewer lines of code :-)


License
-------

ws2812_spi_sparkcore is licensed under the MIT License (see LICENSE.txt).

The only requirement of this license is that you *must include the copyright
and the license text* when you distribute apps using ws2812_spi_sparkcore.

Getting Started
---------------

- Clone the github repository

    `git clone https://github.com/plan44/ws2812_spi_sparkcore`

  or download current snapshot as [ZIP file](https://github.com/plan44/zdetailkit/archive/master.zip)

- Open your spark.io Web IDE, create a new app

- paste the entire contents of the ws2812_spi.cpp file.

- connect a WS2812 based LED strip to the Spark Code, data line on pin A5 (SPI MOSI)

- On line 177, set the number of LEDs you have in the chain (240 is for the commonly available 4m strips with 60 LEDs per meter)

- flash - the sample main() creates a color wheel sweep over the entire LED strip.


(c) 2014 by Lukas Zeller / [plan44.ch](http://www.plan44.ch/opensource.php)







