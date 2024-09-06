package masterServer

import (
	"crypto/md5"
	"fmt"
	"log"
	"net"
	"os"
	"strings"
	"time"
)

// 0 - Caller
// 1~99 - Callee

// data: R:egister
func Register(conn net.Conn, data string) string {
	remoteAddr := conn.RemoteAddr().String()
	fmt.Printf("[ + ] <register> client register: %s\n", remoteAddr)
	ts := time.Now().UnixNano()
	bClientId := md5.Sum([]byte(remoteAddr + "-" + string(ts)))
	clientId := fmt.Sprintf("%x", bClientId)
	fmt.Printf("[ + ] <register> clientId: %s\n", clientId)

	addr := strings.Split(remoteAddr, ":")
	peer := PeerInfo{
		ClientId: clientId,
		Ip:       addr[0],
		Port:     addr[1],
		Conn:     conn,
		TargetId: "null",
	}

	Peers = append(Peers, peer)
	fmt.Printf("[ + ] total: %d\n", len(Peers))
	fmt.Printf("[ + ] new peer:\n")
	fmt.Printf("[ peer ] client id: %s\n", peer.ClientId)
	fmt.Printf("[ peer ] cleint ip: %s\n", peer.Ip)
	fmt.Printf("[ peer ] client port: %s\n", peer.Port)

	var res string
	if len(Peers) < totalPeers {
		PeerMap[clientId] = peer
		res = fmt.Sprintf("%s:%s:%s", clientId, addr[0], addr[1])

	} else if len(Peers) == totalPeers {
		go DispatchRole()
		PeerMap[clientId] = peer
		res = fmt.Sprintf("%s:%s:%s", clientId, addr[0], addr[1])
	} else {
		res = "full:full:full"
	}

	return res

}

func DispatchRole() {
	time.Sleep(3 * time.Second)
	CallerConn := Peers[0].Conn
	CallerConn.Write([]byte("R:Caller"))

	for i := 1; i <= (totalPeers - 1); i++ {
		CalleeConn := Peers[i].Conn
		CalleeConn.Write([]byte("R:Callee"))
	}

}

// data: G:<selfClientId>
func GetAllPeerInfo(data string) string {
	selfClientId := strings.Split(data, ":")[1]
	fmt.Printf("[ + ] <GetAllPeerInfo> request clientId: %s\n ", selfClientId)
	var res = "NoPeer"

	var calleeClientIds []string
	for i := 1; i <= (totalPeers - 1); i++ {
		calleeClientIds = append(calleeClientIds, Peers[i].ClientId)
	}
	res = strings.Join(calleeClientIds, ":")
	fmt.Println("---------------------------------------")
	fmt.Printf("[ + ] total Callee: %d\n", len(calleeClientIds))
	fmt.Printf("[ + ] Callees: %s\n", res)
	fmt.Println("---------------------------------------")

	return res
}

// data: g:<selfClientId>
func GetSinglePeerInfo(data string) string {
	calleeClientId := strings.Split(data, ":")[1]
	fmt.Printf("[ + ] <GetSinglePeerInfo> request clientId: %s\n ", calleeClientId)
	var res = "NoPeer"
	callee := PeerMap[calleeClientId]
	res = "g:" + callee.Ip + ":" + callee.Port
	return res
}

// data: C:<sourceClientId>:<targetClientId>
func Connect(data string) {
	clientIds := strings.Split(data, ":")
	sourceClientId := clientIds[1]
	targetClientId := clientIds[2]
	fmt.Printf("[ + ] <Connect> : %s require to connect to %s\n", sourceClientId, targetClientId)
	NoticeCallee(sourceClientId, targetClientId)
	return
}

func NoticeCallee(source, target string) bool {
	fmt.Println("NoticeCallee")

	conn := PeerMap[target].Conn
	peer := PeerMap[source]

	payload := fmt.Sprintf("P:%s:%s:%s", peer.ClientId, peer.Ip, peer.Port)
	fmt.Println(payload)
	conn.Write([]byte(payload))
	return true
}

// data: A:<CallerClientId>
func Answer(data string) {
	fmt.Println(data)
	CallerClientId := strings.Split(data, ":")[1]
	peer := PeerMap[CallerClientId]
	callerConn := peer.Conn
	callerConn.Write([]byte("Ready"))
	return
}

// data: B:<selfClientId>
func Beat(data string) {

	// selfClientId := strings.Split(data, ":")[1]
	// fmt.Printf("[ + ] heat beat: %s\n", selfClientId)
	return
}

// data: N:<CallerId>:<CallerIp>:<CallerPort>:<CalleeId>:<CalleeIp>:<CalleePort>:<status>
func Note(data string) {
	fmt.Printf("%s\n", data)
	strs := strings.Split(data, ":")
	rec := Record{
		CallerId:   strs[1],
		CallerAddr: strs[2] + ":" + strs[3],
		CalleeId:   strs[4],
		CalleeAddr: strs[5] + ":" + strs[6],
		Status:     strs[7],
	}
	Records = append(Records, rec)
	if len(Records) == (totalPeers - 1) {
		// show and write file

		var recs []string
		for k, v := range Records {
			singleRec := fmt.Sprintf("[ %d ] <%s> %s(%s) -> %s(%s)", k+1, v.Status, v.CallerId, v.CallerAddr, v.CalleeId, v.CalleeAddr)
			recs = append(recs, singleRec)
			fmt.Println(singleRec)
		}
		// write to log file
		f, err := os.Create("tcpLog.txt")
		if err != nil {
			log.Fatal(err)
		}

		for _, line := range recs {
			_, err := f.WriteString(line + "\n")
			if err != nil {
				log.Fatal(err)
			}
		}
		f.Close()
	}
}

// data: T:xxxxx
func Test(data string) string {
	payload := strings.Split(data, ":")[1]
	fmt.Printf("[ + ] <Test> data: %s\n ", payload)
	res := "[ echo ]" + payload
	return res
}
