// I, 20kdc, release this code into the public domain.
// I make no guarantees or provide any warranty,
// implied or otherwise, with this code.

REB_CMD_START("D_SEND", "d_send", reb_cmd_d_send)
REB_CMD_HELP("d_send <channel>, <data>")
REB_CMD_HELP("(actually d_send <cons:channel,data>)")
REB_CMD_HELP("Send a Digilines message.")
REB_CMD_HELP("This uses the first available Digilines IO peripheral to send a Digilines message.")
REB_CMD_EXAMPLE
REB_CMD_HELP("5 d_send \"rtc\", \"GET\"")
REB_CMD_HELP("10 d_recv c$ d$")
REB_CMD_HELP("15 if c$<>\"rtc\" then 10")
REB_CMD_HELP("20 print \"Time: \";d$")
REB_CMD_END

REB_CMD_START("D_RECV", "d_recv", reb_cmd_d_recv)
REB_CMD_HELP("d_recv <channelVar> <dataVar>")
REB_CMD_HELP("Receive a Digilines message.")
REB_CMD_HELP("This uses the first available Digilines IO peripheral to receive a Digilines message.")
REB_CMD_EXAMPLE
REB_CMD_HELP("5 d_send \"rtc\",\"GET\"")
REB_CMD_HELP("10 d_recv c$ d$")
REB_CMD_HELP("15 if c$<>\"rtc\" then 10")
REB_CMD_HELP("20 print \"Time: \";d$")
REB_CMD_END
