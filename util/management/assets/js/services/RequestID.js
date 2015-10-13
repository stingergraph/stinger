app.factory('requestID', function() {
    var id = -1;

    return {
        nextID: function() {
            id++;
            return id;
        }
    }
});