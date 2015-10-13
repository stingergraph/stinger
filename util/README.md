# How to use the util directory
This directory contains several tools to help use STINGER. A Flask script provides an API to perform various commonly-used functions on STINGER, the Graph Explorer provides a web page to monitor algorithm results in real time, the Management Console provides a way to interact with and manage STINGERs directly, and `stingerctl` is a command-line tool to manage the running and logging of STINGER binaries.

## Flask
`stinger_flask.py` is a REST-like API to get data about, into, and out of STINGER. It can be started standalone with `python stinger_flask.py` (use `-h` to see options), or as part of Apache or another web server that supports WSGI. Example Apache configuration below. Requirements for the Flask server can be installed using `pip install -r requirements.txt`. This script was built and tested under Python 2.7.

If using the Apache configuration, it is best to create a user to run STINGER under. `stinger` is used as the username here. Permissions on the `/util` directory are `chown -R www-data:stinger util` and `chmod -R 755 util`, where `www-data` is the user that Apache runs as.

```
Listen 82

<VirtualHost *:82>
    WSGIDaemonProcess stinger user=stinger group=stinger threads=5
    WSGIScriptAlias / /opt/stinger/util/flask/stinger_flask.py
    WSGIScriptReloading On
    WSGIImportScript /opt/stinger/util/flask/stinger_flask.py process-group=%{GLOBAL} application-group=%{GLOBAL}

    <Directory /opt/stinger/util>
        WSGIProcessGroup stinger
        WSGIApplicationGroup %{GLOBAL}
	Require all granted
    </Directory>
</VirtualHost>

<VirtualHost *:80>
	DocumentRoot /opt/stinger/util/management
	<Directory />
	    Options Indexes FollowSymLinks Includes ExecCGI
	    AllowOverride All
	    Require all granted
	</Directory>
	ErrorLog ${APACHE_LOG_DIR}/error.log
	CustomLog ${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
```

Once this is running, Swagger documentation should be visible at `http://localhost:82`.

### Flask Test Scripts
`/util/flask/test` contains a couple scripts that can be useful to test the Flask endpoints or use those endpoints to test various STINGER functionalities.

#### `test_flask.py`
`test_flask.py` is used to insert random edges into STINGER using the Flask `/insert` endpoint. Run it from the console with `python test_flask.py`. Options include number of edges per batch, number of possible edge types, number of possible vertices, and time between batches.

The results of each batch insertion is printed to the console. It includes the success status, batch size, and the time that it took to process the batch.

#### `test_algorithms.py`
`test_algorithms.py` is used to insert a known graph into STINGER, run several algorithms over the set, save the results of those algorithms, and compare the results to another implementation of STINGER. Its purpose is to ensure that changes to STINGER or its algorithms did not effect the results of algorithms.

For now, users must generate the reference values themselves using a known-good implementation of STINGER and its algorithms. TODO: Generate the reference results and compare the current run to those results.

## Management Console
Once the Flask endpoint is running, you can use the STINGER Management Console. To start the Management Console, simply serve the `/util/management` directory using Apache or `python -m SimpleHTTPServer 80`. Configuration options are available in `/util/flask/connections.json`. Once this is running, browsing to `http://localhost/` should display the interface shown below.

### Dashboard
An example of the Management Console dashboard is shown below. It shows Edge Block and Vertex usage, some server and uptime stats, and provies a console to submit RPC Requests.

![STINGER Dashboard Screenshot](http://i.imgur.com/3DlAb0g.png?1)

### Connection Manager
The Connection Manager allows a user to start/stop STINGER or its algorithms through a web interface. It also shows some information about the current connection.

![STINGER Connection Manager](http://i.imgur.com/pZbT57e.png)

## `stingerctl`
This is used to start/stop the STINGER server and algorithms. Some configuration may be required to ensure the paths are correct for the system it is running on. Run `stingerctl` with no flags to see the available options. By default, `stingerctl` saves the running graph on shutdown and reloads the graph on startup.

```
Commands:
    start                 Start stinger.
    stop                  Stop stinger.
    restart               Restart stinger.
    nuke                  Nuke stinger's disk store and restart.
    exec NAME [OPTS]      Execute the provided stinger program NAME with given options.
    addalg NAME [OPTS]    Add the provided algorithm NAME to the running stinger.
    daemon-start          Initializes a watcher process to restart stinger if it crashes.
    daemon-stop           Kill the watcher process.
```
