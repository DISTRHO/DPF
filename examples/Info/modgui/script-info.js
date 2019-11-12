function (event) {

    function magic () {
        if (event.data.magicHasHappened) {
            $('#pedalboard-dashboard').css({
                'background': "#111 url(/img/background.jpg) repeat",
            });
        } else {
            $('#pedalboard-dashboard').css({
                'background': "#111 url(/resources/tilesf1.jpg?uri=http%3A//distrho.sf.net/examples/Info) repeat",
            });
        }
        event.data.magicHasHappened = !event.data.magicHasHappened;
    }

    function handle_event (symbol, value) {
        switch (symbol) {
            case 'time_playing':
            case 'time_validbbt':
                value = value > 0.5 ? "Yes" : "No";
                break;
            case 'time_beatsperminute':
                value = value.toFixed(2);
                break;
            case 'time_frame': {
                var time = value / SAMPLERATE;
                var secs =  time % 60;
                var mins = (time / 60) % 60;
                var hrs  = (time / 3600) % 60;
                event.icon.find('[mod-role=time_frame_s]').text(sprintf("%02d:%02d:%02d", hrs, mins, secs));
                // fall-through
            }
            default:
                value = value.toFixed();
                break;
        }

        event.icon.find('[mod-role='+symbol+']').text(value);
    }

    if (event.type == 'start') {
        var ports = event.ports;
        for (var p in ports) {
            if (ports[p].symbol[0] == ":") {
                continue;
            }
            handle_event (ports[p].symbol, ports[p].value);
        }
        // special cases
        event.icon.find ('[mod-role=sample_rate]').text(SAMPLERATE);
        event.icon.find ('.mod-magic').click(magic);
    }
    else if (event.type == 'change') {
        handle_event (event.symbol, event.value);
    }

}
