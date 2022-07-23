function init() {
    const btnAlarm = document.getElementById("alarm");
    const btnStop = document.getElementById("stopper");
    const btnSubmitPw = document.getElementById("submit-pw");
    const passwordField = document.getElementById("password-input");
    btnAlarm.addEventListener("click", e => {
        e.preventDefault();
        $.post("/alarm");
    });
    btnStop.addEventListener("click", e => {
        e.preventDefault();
        $.post("/stop");
    });
    btnSubmitPw.addEventListener("click", e => {
        e.preventDefault();
        console.log(passwordField.value);
        const validator = new RegExp("^[A-Za-z0-9]{1,20}$");
        const pw = passwordField?.value;
        if (validator.test(pw)) {
            $.ajax({
                type: "post",
                url: "/pw",
                data: pw,
                dataType: "text",
                success: function (response) {
                    if (response === "OK") {
                        btnAlarm.removeAttribute("disabled");
                        localStorage.setItem("pw", "valid");
                    }
                },
            });
        }
        passwordField.value = "";
    });
    if (localStorage.getItem("pw") === "valid") {
        btnAlarm.removeAttribute("disabled");
        btnStop.removeAttribute("disabled");
    }
}
init();
