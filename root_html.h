const char ROOT_HTML[] PROGMEM = R"=====(
<html>
<head>
    <meta name="viewport"
          content="width=device-width, initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, user-scalable=no, minimal-ui">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">

    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootswatch/3.3.7/slate/bootstrap.min.css">
    <link rel="stylesheet"
          href="https://cdnjs.cloudflare.com/ajax/libs/bootstrap-toggle/2.2.2/css/bootstrap-toggle.css">

    <script src="http://code.jquery.com/jquery-3.1.1.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/bootstrap-toggle/2.2.2/js/bootstrap-toggle.min.js"></script>
</head>
<body>
<div class="container">
    <h2 class="device_name">Initialising...</h2>

    <p class="device_description lead">connecting to device..</p>
    <ul class="nav nav-tabs">
        <li class="active"><a data-toggle="tab" href="#control">Control</a></li>
        <li><a data-toggle="tab" href="#status">Status</a></li>
        <li><a data-toggle="tab" href="#alexa">Alexa</a></li>
    </ul>

    <div class="tab-content">
        <div id="control" class="tab-pane fade in active">
            <p></p>

            <div class="well">
                <div class="row">
                    <div class="col-xs-6">
                        <div class="checkbox">
                            <label> <input id="power" class="power live" type="checkbox" checked data-toggle="toggle"
                                           value="">Power</label>
                        </div>
                    </div>
                    <div class="col-xs-6">
                        <div class="checkbox">
                            <label> <input id="auto" class="auto live" type="checkbox" checked data-toggle="toggle" value="">
                                Timer</label>
                        </div>
                    </div>

                </div>
            </div>
            <div class="well">
                <div class="row">
                    <div class="col-xs-12">
                        <label for="brightness">Brightness <span class="glyphicon glyphicon-adjust"
                                                                 aria-hidden="true"></span></label> <input
                            id="brightness" type="range" class="brightness_current" min="0" max="1023"
                            onchange="getViaAjax('set?power=' + this.value);return false;">
                    </div>
                </div>
            </div>
            <div>
                <div class="well">

                    <label>Full beam mode </label>
                    <p class="help-block">Sets the lamp to 100% brightness, then fades down to 1% over the specified duration</p>

                    <div class="btn-group btn-group-justified" role="group" aria-label="...">
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span class="glyphicon glyphicon-time" aria-hidden="true"></span> 15 mins</button>
                        </div>
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span class="glyphicon glyphicon-time" aria-hidden="true"></span> 30 mins</button>
                        </div>
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span class="glyphicon glyphicon-time" aria-hidden="true"></span> 60 mins</button>
                        </div>
                    </div>
                </div>

            </div>

        </div>
        <div id="status" class="tab-pane fade">
            <p/>

            <p>The following events have been configured for <span class="device_name"/>.</p>

            <div class="events well"></div>
        </div>
        <div id="alexa" class="tab-pane fade">
            <p/>

            <p>The following commands have been established for Alexa</p>

            <div class="alexa"></div>

        </div>

    </div>
</div>
<script>
    jQuery.ajax({
        url: "script.js",
        dataType: "script",
        cache: true
    }).done(function () {
        readConfig("config.json");
    });
</script>
</body>
</html>
)=====";
 

