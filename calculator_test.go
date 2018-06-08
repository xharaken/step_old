// To run these tests run: go test *.go
package main

import (
	"math"
	"runtime/debug"
	"testing"
)

// Floating point values have limited precision when adding/dividing,
// so 1.0 + 1.0 might not always be exactly 2.0, depending on the
// value of 1.0 (but it should be pretty close).
const tolerance float64 = 0.000000001

func TestCalculate(t *testing.T) {
	for _, test := range []struct {
		in   string
		want float64
	}{
		{"1+2", 3},
		{"2-1", 1},
		{"1.0+2.1-3", 0.1},
		// Add more test cases here!
	} {
		defer func() {
			if r := recover(); r != nil {
				t.Errorf("Calculate(%v) panicked(%v) but wanted %v", test.in, r, test.want)
				t.Errorf("stacktrace: %s", debug.Stack())
			}
		}()
		got := Calculate(test.in)
		if math.Abs(got-test.want) > tolerance {
			t.Errorf("Calculate(%v) = %v but want %v", test.in, got, test.want)
		}
	}
}
