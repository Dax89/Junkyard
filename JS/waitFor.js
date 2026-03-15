function waitFor(selector, cb, options) {
    const OPTIONS_DEFAULTS = { repeat: false, interval: 800, attempts: -1 };
    options = Object.assign({}, OPTIONS_DEFAULTS, options || {});
    let n = 0;

    const checkNow = function() {
        const nodes = document.querySelectorAll(selector);

        if(!nodes.length) {
            if(options.repeat || options.attempts === -1 || n < options.attempts) {
                setTimeout(checkNow, options.interval);
                n++;
            }
        }
        else {
            nodes.forEach(cb);
            if(options.repeat) setTimeout(checkNow, options.interval);
        }
    }

    checkNow();
}
