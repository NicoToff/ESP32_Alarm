const btnBeep = document.getElementById("beeper");
const btnStop = document.getElementById("stopper");
btnBeep.addEventListener("click", e => {
    e.preventDefault();
    $.post("/beep");
});
btnStop.addEventListener("click", e => {
    e.preventDefault();
    $.post("/stop");
});
