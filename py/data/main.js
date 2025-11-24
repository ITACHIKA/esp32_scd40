var host = window.location.hostname;
var port = window.location.port;
var serverURL = window.location.protocol + "//" + host;
if (port) {
    serverURL += ":" + port;
}

console.log(serverURL);

function setUsereditStatus(status) {
    editableContents = document.getElementsByClassName("userEnter");
    editableNetContents = document.getElementsByClassName("editablediv");
    for (let i = 0; i < editableContents.length; i++) {
        editableContents[i].disabled = status;
    }
    for (var content of editableNetContents) {
        content.contentEditable = !status;
    }
}

document.getElementById('toggleManagement').addEventListener('click', function (event) {
    event.preventDefault();
    document.getElementById("scan").style.display = "none"
    document.getElementById("devlist").style.display = "none"
    document.getElementById("devmanage").style.display = "block"
});

document.getElementById('toggleMonitor').addEventListener('click', function (event) {
    event.preventDefault();
    document.getElementById("scan").style.display = "block"
    document.getElementById("devlist").style.display = "block"
    document.getElementById("devmanage").style.display = "none"
});

document.getElementById('toggleSetting').addEventListener('click', function (event) {
    event.preventDefault();

});

document.getElementById("scan").onclick = async function () {
    let devices = []
    document.getElementById("scan").innerHTML = "Scanning";
    document.getElementById("scan").disabled = true;
    for (let i = 1; i < 256; i++) {
        const timecontrol = new AbortController();
        const timeout = setTimeout(() => timecontrol.abort("Overtime"), 100);
        try {
            ip_str = "192.168.1." + i;
            const res = await fetch("http://" + ip_str + "/esp_discover", { signal: timecontrol.signal });
            clearTimeout(timeout);
            let device = await res.json();
            device.ip = ip_str
            console.log("Found device:", device);
            devices.push(device);
        } catch (e) {
            console.error("Scan failed:", e);
            clearTimeout(timeout);
        }
    }
    document.getElementById("scan").innerHTML = "Discover devices in LAN";
    document.getElementById("scan").disabled = false;
    let selectmenu = document.getElementById("deviceSelect")
    selectmenu.innerHTML = ""
    devices.forEach(d => {
        let option = document.createElement("option")
        option.text = d.ip
        option.value = d.ip
        selectmenu.appendChild(option)
    });
    renderDevices(devices)
    Array.from(document.getElementsByClassName("devcfg-button"))
        .forEach(element => {
            element.addEventListener('click', function (event) {
                event.preventDefault();
                document.getElementById("scan").style.display = "none"
                document.getElementById("devlist").style.display = "none"
                document.getElementById("devmanage").style.display = "block"
                document.getElementById("deviceSelect").value = this.id
            });
        });
}

function formatMicroseconds(us) {

    let totalSeconds = Math.floor(us / 1_000_000);

    let hours = Math.floor(totalSeconds / 3600);
    let minutes = Math.floor((totalSeconds % 3600) / 60);
    let seconds = totalSeconds % 60;

    const pad = (n) => n.toString().padStart(2, '0');

    return `${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;
}

const deviceList = document.getElementById("devlist");

function renderDevices(devices) {
    deviceList.innerHTML = "";
    devices.forEach(d => {
        const card = document.createElement("div");
        card.className = "device-card";
        card.style.margin = "10px"
        const imgpath = "./img/" + d.device + ".jpg"
        card.innerHTML = `
            <div style="border: 2px solid black; padding: 10px;display:flex;align-items:center">
                <img src="${imgpath}" style="height:120px;width:auto;margin-right:20px"></img>
                <div>
                    <devinfo-p>Device type: ${d.device || "ESP32"}</devinfo-p>
                    <br>
                    <devinfo-p>Device name: ${d.name}</devinfo-p>
                    <br>
                    <devinfo-p>Device uptime: ${formatMicroseconds(d.uptime) || "-"}</devinfo-p>
                    <br>
                    <devinfo-p>Device ip: ${d.ip || "-"}</devinfo-p>
                </div>
                <button style="margin-left:auto;align-self:flex-end;" class="devcfg-button" id="${d.ip}">Configure Device</button>
            </div>
        `;
        deviceList.appendChild(card);
    });
}

document.getElementById("sendCommand").onclick = () => {
    ip_addr = document.getElementById("deviceSelect").value;
    ws_str = "ws://" + ip_addr + "/ws";
    let ws = new WebSocket(ws_str);
    ws.onmessage = function (event) {
        console.log("ESP32:", event.data);
        document.getElementById("commandOutput").innerHTML = event.data; //.replace(/\r?\n/g, "<br>");;
        ws.close()
    };
    ws.onclose = () => {
        console.log("WS closed");
    };
    ws.onopen = () => {
        console.log("WS connected");
        let text = document.getElementById("commandInput").value;
        ws.send(text);
    };
}