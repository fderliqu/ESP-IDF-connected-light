function OnButton(){
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET","on",true);
    xmlhttp.send();
}

function OffButton(){
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET","off",true);
    xmlhttp.send();
}