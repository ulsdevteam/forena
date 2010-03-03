<?php

function forena_admin_repos() {
	$form['name'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Name'),
	  '#description' => t('Machine Readable repostiory name.'),
	);
	
	$form['title'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Title'),
	  '#description' => t('Human readable repository name'),
	);
	return $form; 
}