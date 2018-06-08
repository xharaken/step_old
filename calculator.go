// A Go transliteration of
// https://github.com/xharaken/step2015/blob/master/calculator_modularize_2.py
// This is intended to keep the same shape and style of the
// @xharaken's original Python code, but please keep in mind that
// there are much more Go-friendly ways of writing much of this code.
// Consider watching Rob Pike's <r@google.com> talk on using Go to
// write a lexer (lexical scanner) here:
// https://talks.golang.org/2011/lex.slide recorded here
// https://www.youtube.com/watch?v=HxaD_trXwRE
package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"unicode"
)

func main() {
	input := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("> ")
		if !input.Scan() || input.Text() == "" { // Reads a line from standard input.
			return // If it's empty, exit the program.
		}
		answer := Calculate(input.Text())
		fmt.Println("answer =", answer)
	}
}

// Calculate turns a string like "1 + 3" into its corresponding
// numerical value (in this case 4).
func Calculate(line string) float64 {
	tokens := tokenize(line)
	return evaluate(tokens)
}

type token struct {
	// Specifies the type of the token. I'm using the word "kind" here
	// rather than "type" because type is a reserved word in Go.
	kind tokenKind

	// If kind is Number, then number is its corresponding numeric
	// value.
	number float64
}

// TokenKind describes a valid kinds of tokens. This acts kind of
// like an enum in C/C++.
type tokenKind int

// These are the valid kinds of tokens. Each gets automatically
// initialized with a unique value by setting the first one to iota
// like this. https://golang.org/ref/spec#Iota
const (
	Number tokenKind = iota
	Plus
	Minus
)

// Tokenize lexes a given line, breaking it down into its component
// tokens.
func tokenize(line string) []token {
	tokens := []token{token{Plus, 0}} // Start with a dummy '+' token
	index := 0
	for index < len(line) {
		var tok token
		switch {
		case unicode.IsDigit(rune(line[index])):
			tok, index = readNumber(line, index)
		case line[index] == '+':
			tok, index = readPlus(line, index)
		case line[index] == '-':
			tok, index = readMinus(line, index)
		default:
			log.Panicf("invalid character: '%c' at index=%v in %v", line[index], index, line)
		}
		tokens = append(tokens, tok)
	}
	return tokens
}

// Evaluate computes the numeric value expressed by a series of
// tokens.
func evaluate(tokens []token) float64 {
	answer := float64(0)
	index := 0
	for index < len(tokens) {
		switch tokens[index].kind {
		case Number:
			switch tokens[index-1].kind {
			case Plus:
				answer += tokens[index].number
			case Minus:
				answer -= tokens[index].number
			default:
				log.Panicf("invalid syntax for tokens: %v", tokens)
			}
		}
		index += 1
	}
	return answer
}

func readPlus(line string, index int) (token, int) {
	return token{Plus, 0}, index + 1
}

func readMinus(line string, index int) (token, int) {
	return token{Minus, 0}, index + 1
}

func readNumber(line string, index int) (token, int) {
	number := float64(0)
	flag := false
	keta := float64(1)
DigitLoop:
	for index < len(line) {
		switch {
		case line[index] == '.':
			flag = true
		case unicode.IsDigit(rune(line[index])):
			number = number*10 + float64(line[index]-'0')
			if flag {
				keta *= 0.1
			}
		default:
			// "break DigitLoop" here means to break from the labeled for loop, rather than the switch statement. https://golang.org/ref/spec#Break_statements
			break DigitLoop
		}
		index += 1
	}
	return token{Number, number * keta}, index
}
