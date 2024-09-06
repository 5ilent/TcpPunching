package masterServer

import (
	"bufio"
	"fmt"
	"net"
)

type PeerInfo struct {
	ClientId string
	Ip       string
	Port     string
	Conn     net.Conn
	TargetId string
}

var Peers []PeerInfo
var PeerMap = make(map[string]PeerInfo)

type Record struct {
	CallerId   string
	CallerAddr string
	CalleeId   string
	CalleeAddr string
	Status     string
}

var Records []Record

var totalPeers int

func StartMasterServer(listenPort string, number int) {
	totalPeers = number
	listen, err := net.Listen("tcp", "0.0.0.0:"+listenPort)
	if err != nil {
		fmt.Println("[ err ] <server listen fail> ", err)
		return
	}
	fmt.Printf("[ + ] server listen at: %s\n", listenPort)

	for {
		conn, err := listen.Accept()
		if err != nil {
			fmt.Println("[ err ] <server accept fail> ", err)
			continue
		}
		go process(conn)
	}
}

func process(conn net.Conn) {

	defer conn.Close()

	for {
		reader := bufio.NewReader(conn)
		var buf [200]byte
		n, err := reader.Read(buf[:])
		if err != nil {
			fmt.Println("[ err ] <server read fail> ", err)
			break
		}
		data := string(buf[:n])

		var res string
		var echo bool
		switch string(buf[0]) {
		case "R":
			res = Register(conn, data)
			echo = true
		case "G":
			res = GetAllPeerInfo(data)
			echo = true
		case "g":
			res = GetSinglePeerInfo(data)
			echo = true
		case "C":
			Connect(data)
			echo = false
		case "A":
			Answer(data)
			echo = false
		case "B":
			Beat(data)
			echo = false
		case "N":
			Note(data)
			echo = false
		case "T":
			res = Test(data)
			echo = true
		default:
			res = "Unsupported action"
			echo = false
		}
		if echo {
			conn.Write([]byte(res))
		}

	}
}
