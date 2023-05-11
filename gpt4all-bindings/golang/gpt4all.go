package gpt4all

// #cgo CFLAGS: -I../../gpt4all-backend/ -I../../gpt4all-backend/llama.cpp -I./
// #cgo CXXFLAGS: -std=c++17 -I../../gpt4all-backend/ -I../../gpt4all-backend/llama.cpp -I./
// #cgo darwin LDFLAGS: -framework Accelerate
// #cgo darwin CXXFLAGS: -std=c++17
// #cgo LDFLAGS: -lgpt4all -lm -lstdc++
// #include <binding.h>
import "C"
import (
	"fmt"
	"runtime"
	"strings"
	"sync"
	"unsafe"
)

// The following code is https://github.com/go-skynet/go-llama.cpp with small adaptations

type GPTJ struct {
	state unsafe.Pointer
}

func New(model string, opts ...ModelOption) (*GPTJ, error) {
	ops := NewModelOptions(opts...)

	state := C.load_gptj_model(C.CString(model), C.int(ops.Threads))
	if state == nil {
		return nil, fmt.Errorf("failed loading model")
	}

	gpt := &GPTJ{state: state}
	// set a finalizer to remove any callbacks when the struct is reclaimed by the garbage collector.
	runtime.SetFinalizer(gpt, func(g *GPTJ) {
		setCallback(g.state, nil)
	})

	return gpt, nil
}

func (l *GPTJ) Predict(text string, opts ...PredictOption) (string, error) {

	po := NewPredictOptions(opts...)

	input := C.CString(text)
	if po.Tokens == 0 {
		po.Tokens = 99999999
	}
	out := make([]byte, po.Tokens)

	C.gptj_model_prompt(input, l.state, (*C.char)(unsafe.Pointer(&out[0])), C.int(po.RepeatLastN), C.float(po.RepeatPenalty), C.int(po.ContextSize),
		C.int(po.Tokens), C.int(po.TopK), C.float(po.TopP), C.float(po.Temperature), C.int(po.Batch), C.float(po.ContextErase))

	res := C.GoString((*C.char)(unsafe.Pointer(&out[0])))
	res = strings.TrimPrefix(res, " ")
	res = strings.TrimPrefix(res, text)
	res = strings.TrimPrefix(res, "\n")
	res = strings.TrimSuffix(res, "<|endoftext|>")

	return res, nil
}

func (l *GPTJ) Free() {
	C.gptj_free_model(l.state)
}

func (l *GPTJ) SetTokenCallback(callback func(token string) bool) {
	setCallback(l.state, callback)
}

var (
	m         sync.Mutex
	callbacks = map[uintptr]func(string) bool{}
)

//export tokenCallback
func tokenCallback(statePtr unsafe.Pointer, token *C.char) bool {
	m.Lock()
	defer m.Unlock()

	if callback, ok := callbacks[uintptr(statePtr)]; ok {
		return callback(C.GoString(token))
	}

	return true
}

// setCallback can be used to register a token callback for LLama. Pass in a nil callback to
// remove the callback.
func setCallback(statePtr unsafe.Pointer, callback func(string) bool) {
	m.Lock()
	defer m.Unlock()

	if callback == nil {
		delete(callbacks, uintptr(statePtr))
	} else {
		callbacks[uintptr(statePtr)] = callback
	}
}
