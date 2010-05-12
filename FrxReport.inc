<?php
// $Id$
/**
 * @file 
 * Basic report engine.  Controls the rendering of the report. 
 */
define('FRX_NS', 'urn:FrxReports'); 
require_once('FrxSyntaxEngine.inc'); 

class FrxReport { 
  public $rpt_xml; 
  private $cur_data; 
  private $output; 
  private $teng; 
  public $fields;
  public $category; 
  public $access;  
  public $parameters; 
  public $options; 
  public $body; 
  
  public function __construct($xhtml, $data=array()) {
    $this->teng = new FrxSyntaxEngine(FRX_TOKEN_EXP, '{}', $this);
    $this->access = array(); 
    $this->parameters = array(); 
    $this->options = array(); 
    if ($xhtml) { 
      if (!is_object($xhtml)) { 
        $this->rpt_xml = new SimpleXMLElement($xhtml);
      } 
      else {
        $this->rpt_xml = $xhtml; 
      }
      $this->cur_data = $data;
      
      // Load header data
      $rpt_xml = $this->rpt_xml; 
      $this->body = $rpt_xml->body;       
      if ($rpt_xml->head) {
        $this->title = (string)$rpt_xml->head->title; 
                
        foreach ($rpt_xml->head->children(FRX_NS) as $name => $node) { 
          switch ($name) { 
            case 'fields':               
              $this->fields = $node;
              break; 
            case 'category':              
              $this->category = (string)$node; 
              break;     
            case 'options':               
              foreach ($node->attributes() as $key => $value) { 
                $this->options[$key] = (string)$value; 
              }
              break; 
            case 'parameters':              
              foreach ($node->children(FRX_NS) as $key => $node) {
                $parm = array(); 
                foreach ($node->attributes() as $akey => $attr) {
                  $parm[$akey] = (string)$attr; 
                }
                $parm['value']= (string)$node; 
                $this->parameters[$key] = $parm;
              }
              break; 
          }
        }
      }
    }
  }
  
  /**
   * Get the data block 
   * @param $block
   * @return unknown_type
   */
  private function get_data($block) {
    //@TODO: Merge xml data parameters into the report paramters        
    $this->cur_data = forena_invoke_data_engine($block, $this->cur_data); 
  }
  
  private function process_frx_attributes(SimpleXMLElement $node) {
    $attrs = $node->attributes(FRX_NS); 
    if ($attrs) foreach ($attrs as $key => $value) { 
      switch ($key) {
        case "block":
           $this->get_data((string)$value);           
           break;  
      }
    }
  }
  
  /**
   * Recursive report renderer
   * Walks the nodes rendering the report. 
   */
  public function render_section(SimpleXMLElement $node) {     
    $cur_data = $this->cur_data;     
    $elements = count($node->xpath('*'));
    $frx = $node->attributes(FRX_NS);
    // Test to see if we have any nodes that are contains data url    
    if ($node->xpath('*//@frx:*') || $frx) {          
      $attrs = $node->attributes();
      $tag = $node->getName(); 
      $this->process_frx_attributes($node); 
      if ($attrs) foreach ($attrs as $key => $value) { 
        $attr_text =  ' '. $key .'="'. (string)$value .'"';
      }

      $frx = $node->attributes(FRX_NS);
      
      if ((string)$frx['foreach'] ) {       
        // Save xml
        $path = (string)$frx['foreach'];        
        $data = $this->cur_data;   
              
        if ($data) $nodes = $data->xpath($path);
                
        if ($nodes) foreach ($nodes as $x) {
          $this->cur_data = $x;
          $o .= $this->teng->replace('<'. $tag . $attr_text .'>', $cur_data);          
          foreach ($node->children() as $child) { 
            $o .= $this->render_section($child); 
          }
          $o .= '</'. $tag .'>';
        }
        
        $this->cur_data = $data; 
      } 
      else {

        $o .= $this->teng->replace('<'. $tag . $attr_text .'>', $this->cur_data);
        foreach ($node->children() as $child) {
          $o .= $this->render_section($child);  
        }
        $o .= '</'. $tag .'>';
      }
    } 
    else {       

      $tag = $node->getName(); 
      // We can render so lets do it. 
      $text = $node->asXML();      
      $o .= $this->teng->replace($text, $this->cur_data);
    }
    $this->cur_data = $cur_data;  
    return $o; 
  }
  
  /**
   * Render the report
   * @return unknown_type
   */
  public function render($format) {
    $rpt_xml = $this->rpt_xml;
    drupal_set_title($this->title);
    
    if ($rpt_xml->body) $rpt_xml = $rpt_xml->body; 
    $body_xml = $rpt_xml; 
  
    foreach ($body_xml->children() as $node) {
      $o .= $this->render_section($node);   
    }
    
    return $o ;  
  }
  /*
   * Formatter used by the syntax engine to alter data that gets extracted.
   * This invokes the field translation engine 
   */
  public function format($value, $key, $data) {
    // Determine if there is a field overide entry    
    if ($this->fields) {
      $path = 'frx:field[@id="'. $key .'"]';
      $formatters = $this->fields->xpath($path); 
      if ($formatters) foreach ($formatters as $formatter) {        
        if (((string)$formatter['block'] == $this->block) || (!(string)$formatter['block'])) {
          
           //@TODO: Replace the default extraction with something that will get sub elements of the string
           $default = (string)$formatter; 
           $link = (string) $formatter['link']; 
           $format = (string) $formatter['format']; 
           $format_str = (string) $formatter['format-string'];            
        }  
      }
    }
    
    if ($format) { 
      $value = forena_format_data($value, $format, $format_str);
    }
    
    
    // Default if specified
    if (!$value && $default) { 
      $value = $default;  
    }
    
    if ($link) { 
      $link = $this->teng->replace($link,  $data, TRUE);
      list($url, $query) = explode('?', $link); 
      $value = l(htmlspecialchars_decode($value), $url, array('query' => $query));
    }
    return $value; 
  }
  
  /**
   * Return the xml data for the report. 
   *
   * @return unknown
   */
  public function asXML() { 
     if ($this->rpt_xml) { 
       return $this->rpt_xml->asXML(); 
     } 
     else { 
       return '';    
     }
  }
}