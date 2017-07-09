function saveOptions() {
  var weatherIsEnable = document.forms.settings.weather.checked
  var settings = {
    weatherIsEnable: weatherIsEnable
  };
  console.log(settings);
  return settings;
}

$().ready(function () {
  $("#b-cancel").click(function () {
    console.log("Canceling");
    document.location = "pebblejs://close";
  });
  $("#b-save").click(function () {
    console.log("Save");
    var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(saveOptions()));
    console.log(location);
    document.location = location;
  });
});
