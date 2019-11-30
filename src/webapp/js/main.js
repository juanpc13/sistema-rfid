var ws;//var ws = new WebSocket("ws://" + location.hostname + "/ws");
var btn_abrir = $("#btn_abrir");
var btn_agregar = $("#btn_agregar");
var usuario = $("#usuario");
var btn_guardar = $("#btn_guardar");
var btn_cancelar = $("#btn_cancelar");
var estado = $("#estado");
var hexText = $("#hexText");
var hexCode = undefined;

btn_abrir.click(function(){
    var json = {solenoid: true};
    if (ws.readyState == WebSocket.OPEN) {
        ws.send(JSON.stringify(json));
    }
});

btn_agregar.click(function(){
    hexCode = undefined;
    usuario.attr("disabled", true);
    btn_guardar.attr("disabled", true);
    var json = {registrar: true};
    if (ws.readyState == WebSocket.OPEN) {
        ws.send(JSON.stringify(json));
    }
    countDown();
});

var n = 5;
function countDown(){
    if(n > 0){
        if(typeof hexCode !== 'undefined'){
            n = 5;
            return;
        }else{
            setTimeout(countDown,1000);
        }
    }
    hexText.text("Buscando... (" + n + ")");
    n--;
    if(n < 0){
        n = 5;
        btn_cancelar.click();
    }
}

function connect() {
    //ws = new WebSocket("ws://192.168.1.10/ws");
    ws = new WebSocket("ws://" + location.hostname + "/ws");
    ws.onopen = function () {
        // subscribe to some channels
        console.log("WebSocket Open");
        estado.attr("class","badge badge-success");
        estado.text("WebSocket en linea");
    };

    ws.onmessage = function (evt) {
        var json = JSON.parse(evt.data);
        if(typeof json.hex !== 'undefined'){
            hexCode = json.hex;
            hexText.text(hexCode);
            usuario.attr("disabled", false);
            btn_guardar.attr("disabled", false);
        }
        console.log(json);
    };

    ws.onclose = function (evt) {
        console.log('Socket is closed. Reconnect will be attempted in 1 second.', evt.reason);
        setTimeout(function () {
            connect();
            estado.text("WebSocket Reconectando");
        }, 1000);
    };

    ws.onerror = function (err) {
        console.error('Socket encountered error: ', err.message, 'Closing socket');
        estado.attr("class","badge badge-danger");
        estado.text("WebSocket en error");
        ws.close();
    };
}
connect();