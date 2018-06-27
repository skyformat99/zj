package main

import (
	"fmt"
	"os"
	"time"
	"io/ioutil"
	"net/http"
)

func Start() error {
	http.HandleFunc("/test", abc)

	err := http.ListenAndServe(":30000", nil)
	if nil != err {
		fmt.Println(err)
	}

	return nil
}

func main() {
	go func(){
		time.Sleep(10 * time.Second)
		os.Exit(0)
	}()

	Start()
}

func abc(w http.ResponseWriter, r *http.Request) {
	body := "!"

	r.ParseForm()
	if 0 < len(r.Form["name"]) {
		req := r.Form["name"][0]
		body = body + req
	}

	if "POST" == r.Method {
		buf, _ := ioutil.ReadAll(r.Body)
		body = body + string(buf)
		r.Body.Close()
	}

	w.WriteHeader(200)
	_, err := w.Write([]byte(body))
	if nil != err {
		fmt.Println(err)
	}
}
