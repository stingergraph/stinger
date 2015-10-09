
app.controller('MainController',function($scope,$route,$routeParams,$location,pageManagement,stingerConnect) {
    $scope.$route = $route;
    $scope.$location = $location;
    $scope.$routeParams = $routeParams;

    $scope.currentPage = function() {
        $scope.showOverlay = (stingerConnect.initialLoad || stingerConnect.changeConnection) && pageManagement.currentPage == "home";
        return pageManagement.currentPage;
    }

    stingerConnect.initConnection();

    $scope.stingerConnect = stingerConnect;
});
