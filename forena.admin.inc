<?php

function forena_default_repository() { 
  $path = drupal_get_path('module', 'forena'); 
  $repos = array('path' => $path,
          'access' => FALSE,
          'title' => 'Forena Provided Reports'  
         ); 
}

/**
 * List existing repositories and configure new ones.  
 *
 */
function forena_repository_admin($repos='') {
  $repos = variable_get('forena_repositories',forena_default_repository()); 
  
}

