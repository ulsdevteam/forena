<?php
class FrxPrincePDF extends FrxDocument { 
  public function render($r, $format, $options = array()) { 
    include_once('sites/all/libraries/prince.php'); 
    $prince_path = variable_get('forena_pdf_prince_path', '/usr/local/bin/prince'); 
    $disable_links = variable_get('forena_pdf_disable_links', TRUE); 
    $link_class = $disable_links ? 'prince-disable-links': ''; 
    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    if (@$options['css']) {
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }
    
    $output .= '<title>' . $r->title . "</title></head><body class='forena-report $link_class'><h1>" . $r->title . '</h1>' . $r->html . '</body></html>';
    $prince_css = drupal_get_path('module', 'forena_pdf') . '/forena_pdf_prince.css'; 
    // Generate the document
    if (class_exists('Prince') && file_exists($prince_path)) {
      $p = new Prince($prince_path); 
      $p->addStyleSheet($prince_css); 
      foreach (Frx::Skin()->stylesheets as $type=>$sheets) {
        foreach ($sheets as $sheet) { 
          switch ($type) { 
            case 'all': 
            case 'print': 
            case 'screen': 
            case 'pdf':  
              $p->addStyleSheet($sheet); 
              break; 
          }
        }
      }
      header('Content-Type: application/pdf');
      header('Cache-Control:');
      header('Pragma:');
      header("Cache-Control: must-revalidate");  
      
      $p->convert_string_to_passthru($output); 
    }  else {
      drupal_set_message(t('Prince XML Not Properly Installed'), 'error'); 
      return (''); 
    }
  }
}