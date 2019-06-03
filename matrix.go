package main

import (
	"fmt"
	"log"
	"os"
	"strconv"
	"time"
)

func newMatrix(n int) [][]int {
	m := make([][]int, n)
	for i := 0; i < n; i++ {
		m[i] = make([]int, n)
	}
	return m
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("usage: go run matrix.go N")
	}

	// Parse the N argument from the commandline.
	n, err := strconv.Atoi(os.Args[1])
	if err != nil || n <= 0 {
		log.Fatalf("need a positive in for N, not %v", os.Args[1])
	}

	// Initialize our matrices with some values.
	a := newMatrix(n)
	b := newMatrix(n)
	c := newMatrix(n)
	for i := 0; i < n; i++ {
		for j := 0; j < n; j++ {
			a[i][j] = i*n + j
			b[i][j] = j*n + i
			// c is already initialized to all zeros.
		}
	}

	begin := time.Now()

	//////////////////////////////////////////////
	// Write code here to calculate C = A * B
	// (without using numeric computation librarlies like gonum)
	//////////////////////////////////////////////

	end := time.Now()
	fmt.Println("time: ", end.Sub(begin))

	// Print contents of C for debugging.
	fmt.Println(c)

	total := 0
	for i := 0; i < n; i++ {
		for j := 0; j < n; j++ {
			total += c[i][j]
		}
	}

	fmt.Println("sum: ", total)
	// This sum should be 450 for N=3, 3680 for N=4, and 18250 for N=5.
	knownSums := map[int]int{
		3: 450,
		4: 3680,
		5: 18250,
	}
	want := knownSums[n]
	if want != 0 && total != want {
		fmt.Printf("Wanted sum %v but got %v\n", want, total)
	}
}
