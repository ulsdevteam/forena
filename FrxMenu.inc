<?php
class FrxMenu { 
  public $doc_defaults;  // Default Dcoument formats to inlcude
  public $doc_formats;  // Supported document formats
  public $name; // Report name wihout frx extention
  public $language; // Language
  public $format;  //format of report
  public $filename; //Name of file; 
  public $ext; // Extention of file to be returned. 
  public $directory; // name of directory; 
  public $link;   //Link to report in current language
  public $i_Link; //Link to language chaning report name

  public function __construct() { 
    $docs = variable_get('forena_doc_defaults', array());
    
    // Load settings array into normal array
    if ($docs) foreach ($docs as $doc=>$enabled) { 
      if ($enabled) $this->doc_defaults[] = $doc; 
    } else {
      $this->doc_formats = array('web'); 
    }
    
    // Load settings array into normal array
    $docs = variable_get('forena_doc_formats', array(''));
    if ($docs) foreach ($docs as $doc=>$enabled) {
      if ($enabled) $this->doc_formats[] = $doc; 
    }

  }
  
  /**
   * Convert url into file paths based report name.  Load all link data for the 
   * reprot.  Most report urls look like a java lassname, so urls are of the form
   * lang.subdir.anothersubdir.report.doctype.  This function parses the url 
   * into it's components and store them in the menu object so that we can use 
   * this name. 
   * @param $url path style name 
   */
  public function parseURL($url) {
  	global $language;
  	$name = $url;
  	// Determine if the report has an extention that is one of the docuemnt types
  	$p = pathinfo($url); 
    if (array_search($p['extension'], $this->doc_formats)!==FALSE) { 
      $name = $p['filename']; 
      $format = $p['extension']; 
    }
    else {
      $format = 'web'; 
      $ext = ''; 
    }
    
    // Convert class names to directory names. 
    $base_name = str_replace('.', '/', $name); 
    $name = $base_name; 
    $i_name = $base_name; 
  	
    // Determine lanugage from url or drupal language interface. 
  	$lang = $language->language;
    if (module_exists('locale')) {
      
    	//First check to see if the report allready has a language in it
  	  @list($tlang,	$tbase_name) = explode('/', $base_name, 2);
  	  // FInd out if the starting name of the report is an installed language.
  	  $lang_list = language_list();
  	  if (array_key_exists($tlang, $lang_list )) {
  	  	$base_name = $tbase_name;
  	  	if ($lang != $tlang) {
  	      $lang = $tlang;
  	      $language = $lang_list[$lang];
  	      $i_name = $t_lang . '/' . $base_name; 
  	  	}
  	  	if ($tlang == 'en') {
  	  		$name = $base_name = $tbase_name;
  	  	}
  	  }
  	  else {
  	    if ($lang != 'en') {
    	    $name = $lang . '/' . $name; 
  	    }
  	  }
    }
    
    $name = trim(str_replace('.', '/', $base_name),'/'); 
    $filename = $name . '.frx'; 
    
    $desc['name'] = $this->name = $name;
    $desc['directory'] = Frx::File()->directory($filename); 
    $desc['filename'] = Frx::File()->path($filename); 
    $desc['base_name'] = $this->base_name = $base_name; 
    $desc['exists'] = Frx::File()->exists($filename);    
    $desc['link'] = $this->link = 'reports/' . str_replace('/' , '.', $name);
    $desc['i_link']= 'reports/' . $this->i_link = str_replace('/', '.', $i_name); 
    $desc['language'] = $this->language = $lang;
    $desc['format'] = $this->format = $format; 
     
    return $desc;    
  }
  
  /**
   * Generate dcoument links based on report name. 
   * @param unknown_type $docs
   */
  public function doclinks($docs = array()) { 
    // Default documents. 
    if (!$docs) { 
      $docs = $this->doc_defaults; 
    }
  }
}