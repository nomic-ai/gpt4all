package gpt4all

// #cgo CFLAGS: -I${SRCDIR}../../gpt4all-backend/ -I${SRCDIR}../../gpt4all-backend/llama.cpp -I./
// #cgo CXXFLAGS: -std=c++17 -I${SRCDIR}../../gpt4all-backend/ -I${SRCDIR}../../gpt4all-backend/llama.cpp -I./
// #cgo darwin LDFLAGS: -framework Accelerate
// #cgo darwin CXXFLAGS: -std=c++17
// #cgo LDFLAGS: -lgpt4all -lm -lstdc++ -ldl
// void* load_model(const char *fname, int n_threads);
// void model_prompt( const char *prompt, void *m, char* result, int repeat_last_n, float repeat_penalty, int n_ctx, int tokens, int top_k,
//                            float top_p, float temp, int n_batch,float ctx_erase);
// void free_model(void *state_ptr);
// extern unsigned char getTokenCallback(void *, char *);
// void llmodel_set_implementation_search_path(const char *path);
import "C"
import (
	"fmt"
	"runtime"
	"strings"
	"sync"
	"unsafe"
)

// The following code is https://github.com/go-skynet/go-llama.cpp with small adaptations
type Model struct {
	state unsafe.Pointer
}

func New(model string, opts ...ModelOption) (*Model, error) {
	ops := NewModelOptions(opts...)

	if ops.LibrarySearchPath != "" {
		C.llmodel_set_implementation_search_path(C.CString(ops.LibrarySearchPath))
	}

	state := C.load_model(C.CString(model), C.int(ops.Threads))

	if state == nil {
		return nil, fmt.Errorf("failed loading model")
	}

	gpt := &Model{state: state}
	// set a finalizer to remove any callbacks when the struct is reclaimed by the garbage collector.
	runtime.SetFinalizer(gpt, func(g *Model) {
		setTokenCallback(g.state, nil)
	})

	return gpt, nil
}

func (l *Model) Predict(text string, opts ...PredictOption) (string, error) {

	po := NewPredictOptions(opts...)

	input := C.CString(text)
	if po.Tokens == 0 {
		po.Tokens = 99999999
	}
	out := make([]byte, po.Tokens)

	C.model_prompt(input, l.state, (*C.char)(unsafe.Pointer(&out[0])), C.int(po.RepeatLastN), C.float(po.RepeatPenalty), C.int(po.ContextSize),
		C.int(po.Tokens), C.int(po.TopK), C.float(po.TopP), C.float(po.Temperature), C.int(po.Batch), C.float(po.ContextErase))

	res := C.GoString((*C.char)(unsafe.Pointer(&out[0])))
	res = strings.TrimPrefix(res, " ")
	res = strings.TrimPrefix(res, text)
	res = strings.TrimPrefix(res, "\n")
	res = strings.TrimSuffix(res, "<|endoftext|>")

	return res, nil
}

func (l *Model) Free() {
	C.free_model(l.state)
}

func (l *Model) SetTokenCallback(callback func(token string) bool) {
	setTokenCallback(l.state, callback)
}

var (
	m         sync.Mutex
	callbacks = map[uintptr]func(string) bool{}
)

//export getTokenCallback
func getTokenCallback(statePtr unsafe.Pointer, token *C.char) bool {
	m.Lock()
	defer m.Unlock()

	if callback, ok := callbacks[uintptr(statePtr)]; ok {
		return callback(C.GoString(token))
	}

	return true
}

// setCallback can be used to register a token callback for LLama. Pass in a nil callback to
// remove the callback.
func setTokenCallback(statePtr unsafe.Pointer, callback func(string) bool) {
	m.Lock()
	defer m.Unlock()

	if callback == nil {
		delete(callbacks, uintptr(statePtr))
	} else {
		callbacks[uintptr(statePtr)] = callback
	}
}
