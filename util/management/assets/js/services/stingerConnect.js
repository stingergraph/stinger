app.factory('stingerConnect', function($http) {
    var factory = {}

    factory.connection = null;

    factory.stingerURL = '';
    factory.stingerPort = '';
    factory.logPort = '';
    factory.name = '';
    factory.dbURL = 'localhost';

    factory.stingerComponents = {
        'STINGER Server':'stinger_server',
        // 'RPC Server':'stinger_json_rpc_server',
        'PageRank':'stinger_pagerank',
        'Betweenness Centrality': 'stinger_betweenness',
        'K-Core':'stinger_kcore'
    };
    factory.componentsStatus = {};
    factory.componentsRunning = [];

    factory.connections = [
        ['Loading Connections','loading','8080','82']
    ];

    factory.setConnection = function(name) {
        console.log("setConnection called with: " + name);
        factory.changeConnection = true;
        for (var i=0; i < factory.connections.length; i++) {
            if (factory.connections[i].name == name) {
                factory.stingerURL = factory.connections[i].stingerURL;
                factory.stingerPort = factory.connections[i].stingerPort;
                factory.logPort = factory.connections[i].flaskPort;
                factory.name = factory.connections[i].name;
                factory.connection = factory.connections[i];
            }
        }
    };

    factory.pullConnections = function() {
        return $http({method:'GET', url:'http://' + factory.dbURL + ':82/connections'})
            .success(function(data,status,headers,config) {
                factory.connections = data['connections'];
            })
            .error(function(reason) {
                factory.connections = [];
            });
    };

    factory.pullStatus = function() {
        return $http({method:'GET', url:'http://' + factory.dbURL + ':82/status'})
            .success(function(data,status,headers,config) {
                factory.status = data;
                // console.log(factory.status);
                for(component in factory.stingerComponents){
                    factory.componentsStatus[factory.stingerComponents[component]] = factory.status[factory.stingerComponents[component]] === 'RUNNING';
                }
                // console.log(factory.componentsStatus);

            })
            .error(function(reason) {
                factory.status = [];
            });
    };

    factory.toggleService = function(service) {
        console.log(service);
        $http({method:'GET', url:'http://' + factory.dbURL + ':82/toggle/' + service})
            .success(function(data,status,headers,config) {
                // console.log('success');
            })
            .error(function(reason) {
                // console.log('error');
            });
    };

    factory.initConnection = function() {
        factory.initialLoad = true;
        var p = factory.pullConnections();

        p.then(function () {
            factory.initialLoad = false;
            if (factory.connections.length > 0) {
                factory.setConnection(factory.connections[0].name);
                factory.pullStatus();
            }
        });
    };

    return factory;
});

app.factory('pageManagement', function() {
    var factory = {}

    factory.currentPage = 'home';

    return factory;
});
