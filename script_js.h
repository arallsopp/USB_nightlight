const char SCRIPT_JS[] PROGMEM = R"=====(
document.addEventListener("touchstart", function () {
}, !1);
document.addEventListener("touchmove", function () {
}, !1);

$(function () {
    $('#power').change(function () {
        if($(this).hasClass('live')) {
            getViaAjax('set?power=' + (this.checked ? 1023 : 0));
        }else{
            console.log('not live');
        }
        return false;
    });
    $('#auto').change(function () {
        getViaAjax('auto?enabled=' + (this.checked ? 'true' : 'false'));
        return false;
    });
    $('.timer-trigger').click(function () {
        getViaAjax('fade?start=1023&end=1&duration=' + parseInt(this.textContent) * 60);
        return false;
    });

});

function getViaAjax(url) {
    console.log('requesting',url);
    $.ajax({
        url: url,
        success: function (result) {
            window.tmp = result;
            if (typeof(result) !== "object") {
                result = JSON.parse(result);
            }
            updateFromObject(result);
        },
        error: function () {
            alert("failed");
        }
    });
}

function updateByClass(className, newValue) {
    console.log("updating " + className + " to " + newValue);

    switch (className) {

        default:
            $('.' + className).each(function () {
                switch ($(this).attr("type")) {
                    case "checkbox":
                        $(this).removeClass('live')
                               .prop('checked', (newValue == "1"))
                               .change()
                               .addClass('live');
                        break;

                    case undefined:
                        console.log('undefined');
                        $(this).html(newValue);
                        break;
                    case "range":
                        console.log('range');
                        $(this).val(newValue);
                        break;
                    default:
                        console.warn('fall through');
                }

            })
    }
}

function readConfig(url) {
    $.ajax({
        url: url, success: function (result) {
            updateFromObject(result);
        }
    });
}
function createEventsUI(props) {
    console.log("Create events from ", props);
    window.props = props;

    $(props).each(function () {

        var row = $('<div class="row"/>');
        row.append("<div class=\"col-xs-12\"><dl class=\"dl-horizontal\"><dt>" + this.hour + ":" + this.minute + '</dt>'
            + '<dd>' + this.caption + ' targeting ' + this.power + ' over ' + this.duration + ' seconds</dd></dl></div>'
        );
        $('.events').append(row);

    });

}
function createAlexaUI(props) {
    console.log("Creating Alexa UI from ", props);
    window.props = props;


    $(props).each(function () {
        var panel = $('<div class="well"/>');

        panel.append("<h3>"
            + this.name + "</h3>"
            + this.description
            + "<p class='text-right'><em>Alexa! Turn " + this.name + " on.</p>"
        );
        $('.alexa').append(panel);

    });

}

function updateFromObject(obj) {
    for (var property in obj) {
        if (obj.hasOwnProperty(property)) {

            // do stuff
            console.log(property);
            switch (property) {
                case "events":
                    createEventsUI(obj[property]);
                    break;
                case "alexa":
                    createAlexaUI(obj[property]);
                    break;
                default:
                    updateByClass(property, obj[property]);
            }
        }
    }

}

)=====";
 

