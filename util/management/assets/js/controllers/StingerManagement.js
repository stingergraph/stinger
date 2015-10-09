
app.controller('StingerManagement',function($scope,$compile,$http,$interval,requestID,stingerConnect,pageManagement) {
    pageManagement.currentPage = 'home';

    $scope.status = {};

    $scope.stingerConnect = stingerConnect;
    $scope.rpcInput = ['{','"params": {},','  "jsonrpc": "2.0",','  "method": "get_server_health",','  "id": 0','}'].join("\n");

    function createPieChart (divName) {
        var pie = new Highcharts.Chart({
            chart: {
                renderTo: divName,
                margin: [0, 0, 0, 0],
                backgroundColor: null,
                plotBackgroundColor: 'none',

            },

            title: {
                text: null
            },

            tooltip: {
                formatter: function() {
                    return this.point.name +': '+ this.y;

                }
            },
            series: [
                {
                    borderWidth: 2,
                    borderColor: '#F1F3EB',
                    shadow: false,
                    type: 'pie',
                    name: 'Income',
                    innerSize: '65%',
                    data: [
                        { name: '--', y: 0, color: '#b2c831' },
                        { name: '--', y: 1, color: '#3d3d3d' }
                    ],
                    dataLabels: {
                        enabled: false,
                        color: '#000000',
                        connectorColor: '#000000'
                    }
                }]
        });
        return pie;
    }

    function updateEdgeBlockData() {
        var seriesData = [];
        seriesData.push({name: 'Used Edge Blocks', y: $scope.status.result["edge_blocks_in_use"], color: '#b2c831'});
        seriesData.push({name: 'Free Edge Blocks', y: $scope.status.result["max_neblocks"] - $scope.status.result["edge_blocks_in_use"], color: '#3d3d3d'});
        $scope.ebPie.series[0].setData(seriesData, true);
    }

    function updateVertexData() {
        var seriesData = [];
        seriesData.push({name: 'Used Vertices', y: $scope.status.result["vertices"], color: '#b2c831'});
        seriesData.push({name: 'Free Vertices', y: $scope.status.result["max_vertices"] - $scope.status.result["vertices"], color: '#3d3d3d'});
        $scope.vertexPie.series[0].setData(seriesData, true);
    }

    function createCharts() {
        $scope.ebPie = createPieChart("ebPie");
        $scope.vertexPie = createPieChart("vertexPie");
    }

    createCharts();

    $scope.submitRPC = function(payload){
      console.log(payload);
      var config = {"headers": {"Content-Type":"application/x-www-form-urlencoded"}};
      var promise = $http.post('http://' + $scope.stingerConnect.stingerURL + ':' + $scope.stingerConnect.stingerPort + '/jsonrpc/',payload,config);
      promise.then(function(data,status,headers,config) {
          if (data.data.error) {
              $scope.rpcOutput = 'ERROR: Status ' + status + '\n' + data.data;
          } else {
              $scope.rpcOutput = data.data;
          }
      }, function(reason) {
          $scope.status = null;
      });
    }

    function get_health() {
        var args = {"jsonrpc":"2.0", "method":"get_server_health","id":requestID.nextID()};
        var config = {"headers": {
            "Content-Type":"application/x-www-form-urlencoded"
        }};

        var promise = $http.post('http://' + $scope.stingerConnect.stingerURL + ':' + $scope.stingerConnect.stingerPort + '/jsonrpc/',args,config);

        promise.then(function(data,status,headers,config) {
            if (data.data.error) {
                $scope.status = null;
            } else {
                $scope.status = data.data;
            }
        }, function(reason) {
            $scope.status = null;
        });

        return promise;
    };

    function get_algs() {
        if(!$scope.stingerConnect.stingerURL){
          return false;
        }
        var args = {"jsonrpc":"2.0", "method":"get_algorithms","id":requestID.nextID()};
        var config = {"headers": {
            "Content-Type":"application/x-www-form-urlencoded"
        }};

        var promise = $http.post('http://' + $scope.stingerConnect.stingerURL + ':' + $scope.stingerConnect.stingerPort + '/jsonrpc/',args,config);

        promise.then(function(data,status,headers,config) {
            if (data.data.error) {
                $scope.algs = null;
            } else {
                $scope.algs = data.data;
            }
        }, function(reason) {
            $scope.algs = null;
        });

        return promise;
    };


    var p = get_health();
    p.then(function() {
        stingerConnect.changeConnection = false;
        // console.log($scope.status);
        updateEdgeBlockData();
        updateVertexData();
    });

    $scope.logData = "";

    $scope.updateLogData = function() {
        $http({method:'GET', url:'http://' + $scope.stingerConnect.stingerURL + ':' + $scope.stingerConnect.logPort + '/log'})
        .success(function(data,status,headers,config) {
            $scope.logData = data;
        })
        .error(function(reason) {
            $scope.logData = "Error getting logs";
        });
    };

    $scope.updateLogData();

    var last = $scope.stingerConnect.name;

    $interval(function() {
        if ($scope.stingerConnect.name != last) {
            last = $scope.stingerConnect.name;
            createCharts();
            $scope.updateLogData();
        }

        if ($scope.stingerConnect.name == '') {
            return;
        }


        var p = get_health();
        var p2 = get_algs();
        p.then(function() {
            // console.log($scope.status);
            stingerConnect.changeConnection = false;
            updateEdgeBlockData();
            updateVertexData();
        })
    }, 3000);


    $scope.toggleConnection = function(){

    }

});
