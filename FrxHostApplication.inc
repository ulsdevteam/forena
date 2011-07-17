<?php
global $forena_application_class;
$forena_application_class = 'FrxHostApplication';
class FrxHostApplication {
	/**
	 * @file HostApp.inc
	 * Defines all the interface points between the host application and Forena.
	 * Each of these methods must be specified in order for Forena to function properly.
	 * The base class is just here to let you know
	 */

	// This varialbe is a factory variable to make sure we only get one instance of this class
	static private $instance = '';
	public $repositories = Array();

	// A factory method that is used to generate the instance of the application.
	static public function instance() {
		if (self::instance) {
			return new Application();
		}
		else
		  return self::instance;
	}

	/**
	 * Configuration retrieval method
	 * Returns configuration varialbes used by forena.
	 * @param unknown_type $var_name
	 */
  public function configuration($var_name) {
  	 global $forena_conf;
     return $forena_conf;
  }


  /**
   * Convert a relative link to appropriately rendered html
   * return html A properly formatted anchor tag
   */
  public function link($title, $path, $options=array()) {
    return $title;
  }

  /**
   * Theme the output of a css
   * Enter description here ...
   * @param unknown_type $output
   * @param unknown_type $doc_type
   */
  public function theme($output, $doc_type, $title) {
    return $output;
  }

  /**
   * Add a css file for theming.
   * Enter description here ...
   * @param unknown_type $css_file
   */
  public function add_css($css_file) {

  }

  /**
   * Add a javascript file for theming css
   * Enter description here ...
   * @param unknown_type $js_file
   */
  public function add_js($js_file) {

  }

  /**
   * Convert the report name into a link to the report
   * Enter description here ...
   * @param unknown_type $report_name
   */
  public function report_link($report_name, $title) {
    return $this->link($title, $path );
  }

  /**
   * Error Reporting
   */
 public function error($short, $long) {

 }


 /**
 * Determines which css files need to be loaded.
 *
 * @param array $desc Report descriptor from forena_rport_desc
 * @param string $form The report "form" to be used.  From the report
 * @param string $format Document format that will be used for the report.
 * @return array A list of css files that should be applied to the report.
 */
function report_css($desc, $form,  $format = '') {
  $css_files = array();
  // First check for the form file
  $path = forena_report_path();
  if (file_exists($path . '/' . $form . '.css')) {
    $css_files[] = $path . '/' . $form . '.css';
  }
  if ($format && file_exists($path .'/'. $form .'-'. $format .'.css')) {
    $css_files[] = $path . '/'. $form . '-' . $format .'.css';
  }

  // Now check for a report specific file
  $base_file = $path . '/' . $desc['name'];

  if ($format && file_exists($base_file .'-' . $format .'.css')) {
    $css_files[] = $base_file .'-'. $format . '.css';
  }
  elseif (file_exists($base_file .'.css')) {

    $css_files[] = $base_file .'.css';
  }
  //drupal_set_message("desc: <pre>". print_r($desc, 1) ."</pre>");
  //drupal_set_message("css files: <pre>". print_r($css_files, 1) ."</pre>");
  return $css_files;
}

/**
 * Load and render a report based on a drupal path.
 * In this function the arglist is used to get the full path to the report.
 *
 * @return unknown
 */
function forena_report($name_in, $parms = array()) {

  $desc = forena_report_desc();
  $name = $desc['name'];
  $format = $desc['format'];
  $filename = $desc['filename'];
  // Determine the data to get.
  if (!$parms) $parms = $_GET; else $parms = (array)$parms;
  unset($parms['q']);
  if ($name) {
    $r = forena_get_report($name, $parms);
    if ($r) {
    //check for default parameters
    $r_params = $r->parameters;

      if ($r_params) {
        //put default parameters in parms array
        foreach ($r_params as $key => $parm) {
          if (!$parms[$key] && $parm['value']) {
            $parms[$key] = $parm['value'];
            $reload_params = TRUE;
          }
          //do not show report if a required parameter does not have a value
          //force the user to input a parameter
          if ((!$parms[$key]) && (strcmp($parm['require'], "1") == 0) && (!$parm['value'])) {
            $missing_parms = TRUE;
          }
        }
      }
      //Reload report if parameters were missing
      if ($reload_params) {
        $r = forena_get_report($name, $parms);
      }

      // Get data from the report;
      $r_parms = $r->parameters;
      $form = $r->options['form'];
      $rpt_xml = $r->rpt_xml;

      // Default the form
      if (!$form) $form = variable_get('forena_default_form', 'letter');
      $q;

      //put title on top of report
      $title = $r->title;
      if (!$missing_parms) {
        $output .= $r->render($format);
        $css_files = forena_report_css($desc, $form, $format);
      }

      if ($format) { //a format was requested
        $header = '<h1>' . $title . '</h1>';
        $output = $header . $output;
        if ($css_files) foreach ($css_files as $css_file) {
          $css_text .= file_get_contents($css_file);
        }
        $options = array('css' => $css_text,
                         'docname' => str_replace(' ', '_', $title),
                         'xml' => $r_text,
                         'title' => $title[0],
                   );

        $this->generate_doc($format, $output, $options);
      }
      else {
        // We've got parameters so display the parameters form
        if ($r_parms) $output = drupal_get_form('forena_parameters_form') . $output;
        //set the title
        $title = $r->title;
        drupal_set_title($title);
        //Creating links for downloadable documents.
        //build querystring for document href
        foreach ($parms as $key => $value) {
          $q .= "&". $key . '='. $value;
        }
        $q = trim($q, '&');

        //Building the document links
        $rpt_xml = $r->rpt_xml;
        $nodes = $rpt_xml->xpath('//frx:docgen/frx:doc');
        $div .= '<div class="doclinks">';
        $default_doctypes = $this->configuration('forena_doc_formats');
        if (!$missing_parms) {
          if (!$nodes) {
            //show the default. All supported links
            foreach ($default_doctypes as $value) if (is_object(forena_get_doctypes($value))) {
              $div .= '<a class="doclinks" href="'. $name_in . '.'. $value .'?'. $q .'">'. strtoupper($value) .'</a>';
            }
          }
          else {
            //There were nodes. show the prefered doc types
            $doctypes = $this->configuration('supported_doctypes');
            foreach ($nodes as $value) {
              $arr = $value->attributes();
              $type = (string)$arr['type'];
              if ($doctypes[$type]) {
                if (is_object(forena_get_doctypes($type))) {
                  $div .= '<a class="doclinks" href="'. $name_in . '.'. $type .'?'. $q .'">'. strtoupper($type) .'</a>';
                }
              }
            }
          }
          $div .= '</div>';
          $output = $div . '<div class="forena-report">'. $output .'</div>';
          $output = $this->app->theme($output, $title, $format);
        }
        $this->add_css($this->forena_path(). '/forena.css');
        if ($css_files) foreach ($css_files as $css_file) {
          $this->add_css($css_file, 'module');
        }
        return $output;
      }
    }
    else {
    	// Didn't find it so assume we've got a stale cache entry and delete it.
      return $this->not_found();
    }
  }
  else {
    return $this->not_found();
  }
}

public function forena_path() {
	return 'forena';
}

public function not_found($name) {

}

public function repositories() {
	global $_forena_repositories;
	return $_forena_repositories;
}

}