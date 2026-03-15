function waitFor(selector, cb) {
    const check = (root) => {
        root.querySelectorAll(selector).forEach(cb);
        if (root.matches(selector)) cb(root);
    };

    check(document.body);

    new MutationObserver(mutations => {
        for (const m of mutations) {
            for (const n of m.addedNodes) {
                if (n.nodeType !== Node.ELEMENT_NODE) continue;
                check(n);
            }
        }
    }).observe(document.body, { childList: true, subtree: true });
}
