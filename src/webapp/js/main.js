var ws;//var ws = new WebSocket("ws://" + location.hostname + "/ws");
var btn_abrir = $("#btn_abrir");
var btn_agregar = $("#btn_agregar");
var usuario = $("#usuario");
var btn_guardar = $("#btn_guardar");
var btn_cancelar = $("#btn_cancelar");
var estado = $("#estado");
var hexText = $("#hexText");
var hexCode = undefined;
var table_body = $("#table_body");

btn_abrir.click(function () {
    var json = { solenoid: true };
    if (ws.readyState == WebSocket.OPEN) {
        ws.send(JSON.stringify(json));
    }
});

btn_agregar.click(function () {
    hexCode = undefined;
    usuario.attr("disabled", true).removeClass("is-invalid is-valid").val("");
    btn_guardar.attr("disabled", true);
    var json = { registrar: true };
    if (ws.readyState == WebSocket.OPEN) {
        ws.send(JSON.stringify(json));
    }
    countDown();
});

btn_guardar.click(function () {
    if (usuario.val()) {
        usuario.removeClass("is-invalid").addClass("is-valid");
        var json = { "usuario": usuario.val(), "hex": hexCode };
        if (ws.readyState == WebSocket.OPEN) {
            ws.send(JSON.stringify(json));
        }
        cargarTablaDelay();
        btn_cancelar.click();
    } else {
        usuario.removeClass("is-valid").addClass("is-invalid");
    }
});

var n = 5;
function countDown() {
    if (n > 0) {
        if (typeof hexCode !== 'undefined') {
            n = 5;
            return;
        } else {
            setTimeout(countDown, 1000);
        }
    }
    hexText.text("Buscando... (" + n + ")");
    n--;
    if (n < 0) {
        n = 5;
        btn_cancelar.click();
    }
}

function connect() {
    //ws = new WebSocket("ws://192.168.1.8/ws");
    ws = new WebSocket("ws://" + location.hostname + "/ws");
    ws.onopen = function () {
        // subscribe to some channels
        console.log("WebSocket Open");
        estado.attr("class", "badge badge-success");
        estado.text("WebSocket en linea");
        cargarTablaDelay();
    };

    ws.onmessage = function (evt) {
        var json = JSON.parse(evt.data);
        if (typeof json.hex !== 'undefined') {
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
            estado.text("WebSocket Reconectando");
            connect();
        }, 1000);
    };

    ws.onerror = function (err) {
        console.error('Socket encountered error: ', err.message, 'Closing socket');
        estado.attr("class", "badge badge-danger");
        estado.text("WebSocket en error");
        ws.close();
    };
}
connect();

function username(e) {
    if (String.fromCharCode(e.which).match(/^[A-Za-z0-9\x08]$/)) {
        return true;
    }
    return false;
}

function getUsuarios() {
    var list = [];
    $.ajax({
        type: "GET",
        async: false,
        //url: "http://192.168.1.8/data.json",
        url: "data.csv",
        success: function (data) {
            var response = csvJSON(data);
            //var response = jQuery.parseJSON(data);
            if (typeof response === 'undefined') {
                alert("Error al Cargar la Tabla");
            } else {
                list = response;
            }
        }
    });
    return list;
}

function createTableRowWith(value) {
    var tr = document.createElement("tr");
    var td1 = document.createElement("th");
    var td2 = document.createElement("td");
    td1.setAttribute("scope", "row");
    td1.innerText = value.hex;
    td2.innerText = value.usuario;
    tr.append(td1, td2);
    return tr;
}

function cargarTabla() {
    var listTable = getUsuarios();
    if (listTable.length > 0) {
        //Vaciar la tabla
        table_body.html("");
        //Llenar la tabla
        $.each(listTable, function (key, value) {
            var tr = createTableRowWith(value);
            table_body.append(tr);
        });
    } else {
        var emptyValue = { "usuario": "-", "hex": "-" };
        var tr = createTableRowWith(emptyValue);
        table_body.append(tr);
    }
}

function csvJSON(csv) {
    var lines = csv.split("\n");
    var result = [];
    var headers = lines[0].split(",");

    for (var i = 1; i < lines.length; i++) {
        var obj = {};
        var currentline = lines[i].split(",");

        for (var j = 0; j < headers.length; j++) {
            obj[headers[j]] = currentline[j];
        }
        result.push(obj);
    }
    return result; //JavaScript object
    //return JSON.stringify(result); //JSON
}

function cargarTablaDelay() {
    setTimeout(function () {
        cargarTabla();
    }, 1000);
}