var esp32StatFreq = 0;

var host = window.location.hostname;
var port = window.location.port;
var serverURL = window.location.protocol + "//" + host;
if (port) {
    serverURL += ":" + port;
}

console.log(serverURL);

function setUsereditStatus(status){
    editableContents=document.getElementsByClassName("userEnter");
    editableNetContents=document.getElementsByClassName("editablediv");
    for (let i = 0; i < editableContents.length; i++) {
        editableContents[i].disabled=status;
    }
    for(var content of editableNetContents)
    {
        content.contentEditable=!status;
    }
}

setUsereditStatus(1);

document.getElementById('toggleManagement').addEventListener('click', function (event) {
    event.preventDefault(); // 阻止默认的超链接行为

});

document.getElementById('toggleOverview').addEventListener('click', function (event) {
    event.preventDefault(); // 阻止默认的超链接行为

});

document.getElementById('toggleSetting').addEventListener('click', function (event) {
    event.preventDefault(); // 阻止默认的超链接行为

});

