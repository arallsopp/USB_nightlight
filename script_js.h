const char SCRIPT_JS[] PROGMEM = R"=====(


document.addEventListener("touchstart", function () {
}, !1);
document.addEventListener("touchmove", function () {
}, !1);

$(function () {
    $('#power').change(function () {
        if($(this).hasClass('live')) {
            getViaAjax('set?power=' + (this.checked ? 1 : 0));
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

    addQuickFades();

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
function addQuickFades(){
    var div = '';
    for (i=0;i<=100;i=i+10){

        valueForFade = (Math.pow(2,((100-i)/10))-1);
        //create the row
        div = div + "<div class=\"btn-group btn-group-justified\" role=\"group\">";

        //start brightness
        div = div + (
               "<div class=\"btn-group\" role=\"group\">"
             + "<button type=\"button\" data-value=\"" + (valueForFade) + "\" class=\"btn btn-default fade-start\">"
             + "<div style=\"width:" + (100-i) + "%;height:100%;background: #1c1e22;border-radius: 2px;\">&nbsp</div>"
             + "</button>"
             + "</div>"
        );

        //duration
        div = div + (
              "<div class=\"btn-group\" role=\"group\">"
            + "<button type=\"button\" data-value=\"" + ((i+20)/2)*60 + "\" class=\"btn btn-default fade-duration\">"
            + "<span class=\"glyphicon glyphicon-time\"></span> " + (i+20)/2 + " mins</button>"
            + "</div>"
        );

        //end brightness
        div = div + (
        "<div class=\"btn-group\" role=\"group\">"
        + "<button type=\"button\" data-value=\"" + (valueForFade) + "\" class=\"btn btn-default fade-end\">"
        + "<div class=\"pull-right\" style=\"width:" + (100-i) + "%;height:100%;background: #1c1e22;border-radius: 2px;\">&nbsp</div>"
        + "</button>"
        + "</div>"
        );
        //close the row
        div = div + "</div>";
    }
    //add the go button
    div += "<button type=\"button\" class=\"btn btn-default btn-block fade-trigger\">Go</button>";

    $('.quickfades').append(div);
    $('.fade-start').click(function(){
        $('.fade-start').not(this).removeClass('btn-info');
        $(this).toggleClass('btn-info');
    });
    $('.fade-duration').click(function(){
        $('.fade-duration').not(this).removeClass('btn-info');
        $(this).toggleClass('btn-info');
    });
    $('.fade-end').click(function(){
        $('.fade-end').not(this).removeClass('btn-info');
        $(this).toggleClass('btn-info');
    });
    $('.fade-trigger').click(function(){
        var param = [];
        var test = $('.fade-start.btn-info');
        if(test.length){
            param.push("start=" + test.data().value);
        }

        test = $('.fade-end.btn-info');
        if(test.length){
            param.push("end=" + test.data().value);
        }

        test = $('.fade-duration.btn-info');
        if(test.length){
            param.push("duration=" + test.data().value);
        }

        url = "fade?" + param.join("&");
        console.log(url);
        getViaAjax(url);
    })

}

function updateByClass(className, newValue) {
    console.log("updating " + className + " to " + newValue);

    switch (className) {
        case "time_h":case "time_m":case "time_s":
        newValue = pad(newValue,2);
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
                        console.log('does not have a type');
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

    $(props).each(function () {

        var row = $('<div class="row"/>');
        row.append("<div class=\"col-xs-12\"><dl class=\"dl-horizontal\"><dt>" + pad(this.hour,2) + ":" + pad(this.minute,2) + '</dt>'
            + '<dd>Event <span class="badge">' + this.caption + '</span> transitions to <span class="badge"><span class=\"glyphicon glyphicon-adjust\"></span> ' +   this.power + '</span> over <span class="badge">' + this.duration + '</span> seconds</dd></dl></div>'
        );
        $('.events').append(row);
    });
}

function pad(value, length) {
    return (value.toString().length < length) ? pad("0"+value, length):value;
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
 

