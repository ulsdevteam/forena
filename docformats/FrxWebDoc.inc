<?php
/**
 * Standard web document manager
 * @author metzlerd
 *
 */
class FrxWebDoc extends FrxDocument { 
  
  public function render($r, $format, $options=array()) { 
    // Set title based on report.  
    if ($r->title) { 
      drupal_set_title(filter_xss($r->title));  
    }
    
    // Add Style attribute from header
    
    
    // Add css files
    foreach (Frx::Skin()->stylesheets as $type=>$sheets) {
      foreach ($sheets as $sheet) { 
        switch ($type) { 
          case 'all': 
          case 'print': 
          case 'screen': 
            drupal_add_css($sheet, array('media' => $type)); 
            break; 
        }
      }
    }
    
    // Add javascript files
    foreach (Frx::Skin()->scripts as $script) {
      drupal_add_js($script); 
    }
    
    // Build the parameters form
    $parameters=''; 
    $f = drupal_get_form('forena_parameter_form', $r->rpt_xml->asXML(), $r->blocks_loaded);    
    if ($f) $parameters = drupal_render($f);
  
    // @TODO: Refactor to support theming of document links. 
    // Build Links for document. 

    $formats = ''; 
    if ($r->html) {
      //Building the document links
      $rpt_xml = $r->rpt_xml;
      $nodes = $rpt_xml->xpath('//frx:docgen/frx:doc');
      $formats = '<div class="doclinks">';
      $default_doctypes = variable_get('forena_doc_defaults', array());

      if (!$nodes) {
        //show the default.
        if ($default_doctypes) foreach ($default_doctypes as $value) {
          $formats .= @l(strtoupper($value) . ' ', 'report_doc/' . $name_in . '.' . $value, array('query' => $r->parms, 'class' => 'doclinks'));
        }
      }
      else {
        //There were nodes. show the prefered doc types
        $doctypes = FrxReportGenerator::instance()->supported_doctypes();
        foreach ($nodes as $value) {
          $arr = $value->attributes();
          $type = (string)$arr['type'];
          if (@$doctypes[$type]) {
            if (is_object($this->get_doctypes($type))) {
              $formats .= l(strtoupper($type) . ' ', 'report_doc/' . $name_in . '.' . $type, array('query' => $parms, 'class' => 'doclinks'));
            }
          }
        }
      }
    }
    $formats .= '</div>';   

    
    // Use a theme function to make sure the report gets themed. 
    $variables = array(
      'parameters' => $parameters, 
      'doc_links' => $formats, 
      'content' => $r->html, 
    );
   
    $output = theme('forena_web_report', $variables); 
   
    return $output; 
  }
 
  
}