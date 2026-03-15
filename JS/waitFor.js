function waitFor(selector, cb, repeat) {
    const check = (root) => {
        const nodes = root.querySelectorAll(selector);
        nodes.forEach(cb);

        if(root.matches(selector)) cb(root);
        return nodes.length > 0 || root.matches(selector);
    };

    if(check(document.body) && !repeat) return;

    const observer = new MutationObserver(mutations => {
        let found = false;

        for(const m of mutations) {
            for(const n of m.addedNodes) {
                if(n.nodeType !== Node.ELEMENT_NODE) continue;
                if(check(n)) found = true;
            }
        }

        if(found && !repeat) observer.disconnect();
    });

    observer.observe(document.body, {
        childList: true,
        subtree: true,
    });
}
