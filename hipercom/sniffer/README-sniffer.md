
# Sniffer Firmware for the Zolertia Z1


Author: Cedric Adjih, Inria

Licensing: 2 clause-BSD for sniffer code itself - see header of each file 
(note that Contiki itself is under 3 clause-BSD).

---------------------------------------------------------------------------

Sniffer
-------

The sniffer firmware:
- allows to use a [Zolertia Z1](http://zolertia.com/products/z1) as a sniffer for 802.15.4 packets
- it is based on Contiki (using it mainly as a bootstrapper and for its radio driver)
- associated programs can forward output to [Wireshark](https://www.wireshark.org/)

---------------------------------------------------------------------------

Downloading sniffer code
------------------------

* The code is available in the project [contiki-hiper](http://gforge.inria.fr/projects/contiki-hiper/) of Inria Gforge.
* It is retrieved from git (as indicated there) with:

  `git clone https://gforge.inria.fr/git/contiki-hiper/contiki-hiper.git`

* The sniffer itself lies in the `sniffer` branch, and therefore you
  need to switch to that branch to see the code:
  ```
  cd contiki-hiper
  git checkout sniffer
  ```

* Then the sniffer code itself and related files can be found in  `contiki-hiper/hipercom/sniffer`

---------------------------------------------------------------------------

Building firmware for the mote
------------------------------

First go in the directory `contiki-hiper/hipercom/sniffer`

* To build only the sniffer firmware:

  `make`

  (the firmware will then be found in `sniffer.z1`)

* To build and flash all the motes (as with usual, for Zolertia Z1)
  
  `make sniffer.upload`

* To build and flash some motes:
  
  `make sniffer.upload MOTES=1`

---------------------------------------------------------------------------

Running the sniffer
-------------------

* Running the sniffer as text dump

  . The following command runs a sniffer, connecting to mote at `/dev/ttyUSB0`,
  and dumps the content of all packets:

    `python moteSniffer.py --tty /dev/ttyUSB0 sniffer-text`

* Running the sniffer for Wireshark

  - The following command runs a sniffer, connecting to mote at `/dev/ttyUSB0`,
  and forwarding all the packets to the ZEP port at localhost:

    `python moteSniffer.py --tty /dev/ttyUSB0 sniffer-wireshark`

  - Wireshark can then be run as:

    `wireshark -i lo`

  - Options are available for 'moteSniffer.py'

     - `--no-reset` 

        do not reset the mote before switching to sniffer mode

     - `--channel <channel>`

        switch to channel <channel> for capture

     - `--high-speed`

       switch the serial to high speed (2 Mbps) instead of default 115200 kbps

(the Makefile can be edited so that `make sniff`, `make sniff-fast`, 
 `make rssi`, `make rssi-fast`, `make rssi-dac` and `make flash` are useful)

* Running the sniffer for recording packets:

  - this is documented in [README-record.md](README-record.html)
  
  - Java library reading these records is in `contiki-hiper/hipercom/LogParser`

* LEDS:
  - The sniffer mote starts with all leds off
  - When put in sniffer mode the red led is on, and the blue led is switched
    every time a packet is received

* There are undocumented modes to measure RSSI and/or output information on the
  GPIO/DAC of the Zolertia (useful with an oscilloscope or logical analyzer
  for instance).

* Internals (including the serial port protocol) are documented in 
   [README-internals.md](README-internals.html)

* BUGS:
  - the SFD timestamp could be more precise
  - there are problems with some Zolertia (some without external antennas)
    to change channels

---------------------------------------------------------------------------

Installing one the latest versions of Wireshark
-----------------------------------------------

As of November 2014, the following it is not necessary (nor even accurate):

> As of March 2012, one Ubuntu PPA is available for Wireshark, 
> it is at https://launchpad.net/~dreibh/+archive/ppa
>
> Installation is a the installation of a normal ppa, e.g.:
```
  sudo add-apt-repository ppa:dreibh/ppa
  echo "deb http://ppa.launchpad.net/dreibh/ppa/ubuntu lucid main" >> /etc/apt/sources.list
  echo "deb-src http://ppa.launchpad.net/dreibh/ppa/ubuntu lucid main" >> /etc/apt/sources.list
  sudo apt-get update
  sudo apt-get install wireshark
```
---------------------------------------------------------------------------
