/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

package main

import (
	"crypto/tls"
	"encoding/hex"
	"fmt"
	"io"
	"log"
	"time"
)

func main() {
	// Load the server certificate and private key
	cert, err := tls.LoadX509KeyPair("server_cert.pem", "server_key.pem")
	if err != nil {
		log.Fatal("Error loading certificate:", err)
	}

	// Configure TLS
	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{cert},
	}

	// Create a listener for raw TLS connections
	listener, err := tls.Listen("tcp", ":8443", tlsConfig)
	if err != nil {
		log.Fatal("Error creating TLS listener:", err)
	}
	defer listener.Close()

	fmt.Println("TLS server listening on port 8443...")

	// Accept incoming connections
	for {
		log.Println("TLS elo IN:")
		conn, err := listener.Accept()
		if err != nil {
			log.Println("Error accepting connection:", err)
			continue
		}

		tlsConn := conn.(*tls.Conn)
		err = tlsConn.Handshake()
		if err != nil {
			log.Println("TLS handshake failed:", err)
			tlsConn.Close()
			continue
		}

		buffer := make([]byte, 256)

		for {
			deadline := time.Now().Add(10 * time.Second)
			conn.SetReadDeadline(deadline)
			_, err := conn.Read(buffer)
			if err != nil {
				if err == io.EOF {
					break
				}
			}

			log.Printf("Recv=%s", hex.Dump(buffer))

			io.WriteString(conn, "Hello, raw TLS world!\n")
			time.Sleep(3 * time.Second)
		}

		err = tlsConn.Close()
		if err != nil {
			log.Println("Close failed:", err)
		}

		log.Println("Closed")
	}
}
