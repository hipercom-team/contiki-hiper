---------------------------------------------------------------------------

Internals
---------

The protocol is as follows:
* The local machine sends commands to the mote ('Command Invoke') as:

    `<CODE1 (1 byte)> <CODE2 (1 byte)> <LEN (1 byte)><COMMAND ...>`

  where:
```
    <CODE1>              = 0x43 ('C')
    <CODE2>              = 0x49 ('I')
    <LEN>                = length of command
    <COMMAND INVOKE...>  = the command with LEN bytes 
```

* The mote sends answers to the local machine ('Command Answer') as:

    `<CODE1 (1 byte)> <CODE2 (1 byte)> <LEN (1 byte)><INFO ...>`

  where:
```
    <CODE1>              = 0x43 ('C')
    <CODE2>              = 0x41 ('A')
    <LEN>                = length of command
    <COMMAND ANSWER ...> = the command with LEN bytes 
```

--------------------------------------------------

Most of the time, the first byte of the `<COMMAND INVOKE ...>` defines the 
type of the command, we write it as `<COMMAND CODE>`.

The protocol for each command is as follows:

* Silent stop command: 
   - command without argument i.e. `<COMMAND INVOKE ..> == ''`
   - the mote stops what it was doing, and waits for the next command
   - no output

* Packet sniffer command:
   - Put the mote in sniffer mode, after what it will sends
   - Protocol:
     * -> `<CI-HEADER><COMMAND INVOKE == 'P'>`
     * <- `<CA-HEADER><COMMAND ANSWER == 'P'>`
     * for each packet received, a command answer is sent as:

       <- `<CA-HEADER><COMMAND ANSWER == 'p'><capture-header><packet-data>`

    where `<capture-header>` is:

      `<lost (1)><rssi (1)><link-qual (1)><packet index(4)><timestamp (4)>`

    with:

      `<lost>` is the number of packets lost because serial buffer was full

      `<rssi>` is the RSSI provided by the CC2420

      `<link-qual>` is the link quality provided by the CC2420

      `<packet index>` is a counter of the number of packets

      `<timestamp>` is the number of clock cycles (e.g. at 8 Mhz or 32KHz).

* Channel, Rssi, Rssi-to-Dac, High-speed-serial commands also exist

---------------------------------------------------------------------------

TODO
----

- handle uart errors (e.g.: URCTL1 & RXERR etc...)
  (already did for TelosB:  U1TCTL &= ~URXSE;)
- maybe undo more of cpu/msp430/f1xxx/uart1.c uart1_init 
  DMA initialisation (TelosB)
- port on TelosB

---------------------------------------------------------------------------
