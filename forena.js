
  Drupal.behaviors.forenaCallback = function(context) {
    forenaCallback(Drupal.settings.forena.form, context); 
    forenaCallback(Drupal.settings.forena.report, context); 
  };



function forenaCallback(fnName, context) { 
  fn = window[fnName];
  fnExists = typeof fn === 'function';
  if(fnExists) {
	  fn(context);
  }
}
