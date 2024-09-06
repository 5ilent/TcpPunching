package main

import (
	"fmt"
	"os"
	"strconv"
	"sync"
	"tcpPunchingMaster/masterServer"
)

const LISTEN_PORT = "5555"

var totalNum = 0

func main() {
	fmt.Println("[ * ] start ...")

	totalNum, _ = strconv.Atoi(os.Args[1])

	var wg sync.WaitGroup
	go masterServer.StartMasterServer(LISTEN_PORT, totalNum)
	wg.Add(1)
	wg.Wait()

	fmt.Println("[ * ] exit !")
}
