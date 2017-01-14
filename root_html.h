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
        <li class="active"><a data-toggle="tab" href="#home"><span class="glyphicon glyphicon-home"></span>
            Home</a></li>
        <li><a data-toggle="tab" href="#fades"><span class="glyphicon glyphicon-eye-close"></span> Fades</a></li>

        <li role="presentation" class="pull-right dropdown">
            <a class="dropdown-toggle" data-toggle="dropdown" href="#" role="button" aria-haspopup="true" aria-expanded="false">
                More <span class="caret"></span>
            </a>
            <ul class="dropdown-menu">
                <li><a data-toggle="tab" href="#status"><span class="glyphicon glyphicon-cog"></span> Status</a></li>
                <li><a data-toggle="tab" href="#alexa"><span class="glyphicon glyphicon-comment"></span> Alexa</a></li>
            </ul>
        </li>
    </ul>

    <div class="tab-content">
        <div id="home" class="tab-pane fade in active">
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
                            <label> <input id="auto" class="auto live" type="checkbox" checked data-toggle="toggle"
                                           value=""> Timer</label>
                        </div>
                    </div>

                </div>
            </div>
            <div class="well">
                <div class="row">
                    <div class="col-xs-12">
                        <label for="brightness"><span class="glyphicon glyphicon-adjust" aria-hidden="true"></span>
                            Brightness</label> <input id="brightness" type="range" class="brightness_current" min="0"
                                                      max="1023"
                                                      onchange="getViaAjax('set?brightness=' + this.value);return false;">
                    </div>
                </div>
            </div>
            <div>
                <div class="well">

                    <label><span class="glyphicon glyphicon-eye-open"></span> Full beam mode </label>

                    <p class="help-block">Sets the lamp to 100% brightness, then fades down to 1% over the specified
                        duration</p>

                    <div class="btn-group btn-group-justified" role="group" aria-label="...">
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span
                                    class="glyphicon glyphicon-time" aria-hidden="true"></span> 15 mins
                            </button>
                        </div>
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span
                                    class="glyphicon glyphicon-time" aria-hidden="true"></span> 30 mins
                            </button>
                        </div>
                        <div class="btn-group" role="group">
                            <button type="button" class="btn btn-default timer-trigger"><span
                                    class="glyphicon glyphicon-time" aria-hidden="true"></span> 60 mins
                            </button>
                        </div>
                    </div>
                </div>

            </div>

        </div>
        <div id="fades" class="tab-pane fade ">
            <p></p>

            <div>
                <div class="well">


                    <div class="quickfades"></div>
                </div>

            </div>

        </div>
        <div id="status" class="tab-pane fade">
            <p/>

            <p>The following events have been configured for <span class="device_name"/>.</p>

            <div class="events well"></div>
            <hr/>
            <p class="small pull-right">Loaded at <span class="time_h"></span>:<span class="time_m"></span>:<span
                    class="time_s"></span> from IP:<span class="local_ip"></span>.</p>

        </div>
        <div id="alexa" class="tab-pane fade">
            <p/>

            <p>The following devices have been established for Alexa</p>

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
 

