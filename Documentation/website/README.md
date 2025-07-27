To build the docs, you'll need sphinx and a few extensions that are listed in `conf.py`.

A simple way to get a working setup is to install `pipx`, then run:

```
$ pipx install sphinx
$ pipx inject sphinx {the extensions}
```

Afterwards, building the docs is a single command: `sphinx-build $srcdir $destdir`
