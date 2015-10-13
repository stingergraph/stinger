
app.controller('ConnectionManager',function($scope,$compile,$http,$interval,requestID,stingerConnect,pageManagement) {
    pageManagement.currentPage = 'connections';


    $interval(function() {
        $scope.stingerConnect.pullStatus();
    },2000);


});
