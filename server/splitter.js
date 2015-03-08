exports.formValues = function(data){
  var hash = {};
  if ( (null === data) || (undefined === data) ) {
    return hash;
  }
  var splits = data.split('&');
  for (i = 0; i < splits.length; i++)
  {
    var iSplit = splits[i].split('=');
    hash[iSplit[0]] = iSplit[1];
  }
  return hash;
}
