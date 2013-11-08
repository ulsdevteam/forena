<?php
// $Id$
/**
 * @file
 * Basic report provider.  Controls the rendering of the report.
 */
define('FRX_NS', 'urn:FrxReports');
require_once('FrxSyntaxEngine.inc');
require_once('FrxRenderer.inc');

class FrxReport {
  public $blocks_loaded;
  public $rpt_xml;
  public $fields;
  public $category;
  public $descriptor;
  public $form;
  public $access;
  public $parameters;
  public $options;
  public $formats;
  public $title;
  public $frx_title;
  public $body;
  public $html;
  public $skin;
  private $ids;
  private $data_passed = FALSE;
  public $teng;
  public $parms;
  public $missing_parms = FALSE;
  private $dom;
  private $format;
  private $link_mode = '';


  public function __construct($xhtml, $data=array()) {
    $this->access = array();
    $this->parameters = array();
    $this->options = array();
    $this->parms = $data;
    if ($data) $this->data_passed = TRUE;
    $this->teng = new FrxSyntaxEngine(FRX_TOKEN_EXP, '{}', $this);
    if ($xhtml) {
      $dom = $this->dom = new DOMDocument('1.0', 'UTF-8');
      // Old assumption is an ojbect is a simplexml one
      if (is_object($xhtml)) $xhtml = $xhtml->asXML();

      // Load document and simplexml representation
      $dom->loadXML($xhtml);
      $dom->formatOutput = TRUE;
      $rpt_xml = $this->rpt_xml = simplexml_import_dom($dom);


      // Load header data
      $this->body = $rpt_xml->body;
      if ($rpt_xml->head) {
        $this->title = (string)$rpt_xml->head->title;
        $nodes = $rpt_xml->xpath('//frx:docgen/frx:doc');
        $this->formats = array();
        if ($nodes) foreach ($nodes as $value) {
          $arr = $value->attributes();
          $this->formats[] = (string)$arr['type'];
        }


        foreach ($rpt_xml->head->children(FRX_NS) as $name => $node) {
          switch ($name) {
            case 'fields':
              $this->fields = $node;
              break;
            case 'category':
              $this->category = (string)$node;
              break;

            case 'descriptor':
              $this->descriptor = (string)$node;
              break;
            case 'options':
              foreach ($node->attributes() as $key => $value) {
                $this->options[$key] = (string)$value;
              }
              break;
            case 'title':
              $this->frx_title = (string)$node;
              break;
            case 'form':
              $this->form = (string) $value;
              break;
            case 'parameters':
              foreach ($node->children(FRX_NS) as $key => $node) {
                $parm = array();
                foreach ($node->attributes() as $akey => $attr) {
                  $parm[$akey] = (string)$attr;
                }
                $id = $parm['id'];
                $val = isset($parm['value']) ? $parm['value'] : '';
                $parm['value']= ((string)$node) ? (string)$node : $val;
                $this->parameters[$id] = $parm;
              }
              break;
            case 'doctypes':
              $this->doctypes = $value;
              break;
          }
        }

        $this->skin = isset($this->options['skin']) ? $this->options['skin'] : @$this->options['form'];
      }
    }
  }

  function __destruct() {
    foreach ($this as $key => $value) {
      unset($this->$key);
    }
  }


  /**
   * Get the data block
   * @param $block
   * @return unknown_type
   */
  private function get_data($block, $clause='', $id='', $data_uri='') {
    if ($data_uri) {
      parse_str($data_uri, $data);
      if (is_array($data)) foreach ($data as $key => $value) {
        $data[$key] = $this->teng->replace($value, TRUE);
      }

      // Merge in current_context
      $data = array_merge(Frx::Data()->currentContextArray(), $data);
      $id .= '-parm';
      Frx::Data()->push($data, $id);
    }

    $xml = Frx::RepoMan()->data($block, NULL);
    if ($data_uri) Frx::Data()->pop();

    if ($xml) {
      $this->blocks_loaded = TRUE;
    }
    return $xml;
  }

  /**
   * Recursive report renderer
   * Walks the nodes rendering the report.
   */
  public function render_section(DOMNode $dom_node) {
    $skin = Frx::Data()->getContext('skin');
    $settings = isset($skin['FrxReport']) ? $skin['FrxReport'] : array();
    $continue = TRUE;
    $is_data_block = FALSE;
    $node_type = $dom_node->nodeType;
    $o = '';

    // Shortcut process a text node
    if ($node_type == XML_TEXT_NODE
    || $node_type == XML_ENTITY_REF_NODE
    || $node_type == XML_ENTITY_NODE
    ) {     $text = $dom_node->textContent;
    $o .= $this->teng->replace($text);
    return $o;
    }

    //Handle comment nodes
    if ($node_type == XML_COMMENT_NODE) {
      if (!empty($dom_node->length) &&
      !empty($dom_node->data)) {
        $text = $dom_node->data;
        // comment markup is stripped so need to add it back in
        $o .= '<!--' . $this->teng->replace($text) . '-->';
        return $o;
      } else {
        return '';
      }
    }

    // Continue processing non text nodes
    $node = simplexml_import_dom($dom_node);
    // Special catch to make sure we don't process bad nodes
    if (!is_object($node)) {
      return '';
    }


    $frx = $node->attributes(FRX_NS);
    $include_root = !isset($frx['skip_root']) || !$frx['skip_root'];
    $elements = $dom_node->childNodes->length;

    // Check for invalid link processing.
    if (@(string)$frx['invalid_link']) {
      $old_link_mode = $this->link_mode;
      $this->link_mode = (string)$frx['invalid_link'];
    }

    // Test to see if we have any nodes that contain data url
    if ($node->xpath('*//@frx:*') || $frx) {
      $attrs = $node->attributes();
      $id = (string)$attrs['id'];
      $frx = $node->attributes(FRX_NS);
      $tag = $node->getName();

      if ((string)$frx['block']) {
        $is_data_block = TRUE;
        $xml = $this->get_data((string)$frx['block'], (string)$frx['clause'], $id, (string)$frx['parameters']);
        if ($xml) {
          Frx::Data()->push($xml, $id);
        }
        else {
          return '';
        }
      }

      //Implment if then logic
      if ((string)$frx['if']) {
        $cond = (string)$frx['if'];
        if (!$this->teng->test($cond)) return '';
      }

      // Preserve non frx attributes
      $attr_text='';
      $tmp_attrs = array();
      if ($attrs) foreach ($attrs as $key => $value) {
        $attr_text .=  ' ' . $key . '="' . (string)$value . '"';
        $tmp_attrs[$key] = (string)$value;
      }

      // Determine if we have a custom renderer
      $renderer = (string)$frx['renderer'];
      // if we have a foreach in this node, we need to iterate the children
      if ((string)$frx['foreach'] ) {
        // Save xml
        $path = $this->teng->replace((string)$frx['foreach'], TRUE);
        $data = Frx::Data()->currentContext();

        if (is_object($data)) {
          if (method_exists($data, 'xpath')) {
            $nodes = $data->xpath($path);
          }
          else {
            $nodes = $data;
          }
        }
        else {
          $nodes = (array)$data;
        }
        $i=0;

        //$tmp_attrs = (array)$attrs;

        if ($nodes) foreach ($nodes as $x) {
          Frx::Data()->push($x, $id);
          $i++;
          $odd = $i & 1;
          $row_class = $odd ? 'odd' : 'even';
          $r_attr_text = '';

          if (isset($attrs['id'])) {
            if (strpos($attrs['id'],'{')!== FALSE) {
              $id = $this->teng->replace($attrs['id']);
            }
            else {
              $id = $attrs['id'] . '-' . $i;
            }
            $tmp_attrs['id'] =  $id;
          }

          if (@!$settings['noHelperClasses']) {
            $tmp_attrs['class'] = trim($attrs['class'] . ' ' . $row_class);
          }

          foreach ($tmp_attrs as $key => $value) {
            $r_attr_text .=  ' ' . $key . '="' . (string)$value . '"';
          }

          if ($include_root) $o .= $this->teng->replace('<' . $tag . $r_attr_text . '>');
          foreach ($dom_node->childNodes as $child) {
            $o .= $this->render_section($child);
          }
          if ($include_root) $o .= '</' . $tag . '>';
          Frx::Data()->pop();
        }
      }
      elseif ($continue) {
        if ($renderer) {
          // Implement custom renderer.
          $co = Frx::Controls($renderer);
          if ($co) {
            $co->initReportNode($dom_node, $this, $this->format);
            $o = $co->render();

          }
        }
        else {
          if ($include_root) $o .= $this->teng->replace('<' . $tag . $attr_text . '>');

          // None found, so render children
          foreach ($dom_node->childNodes as $child) {
            $o .= $this->render_section($child);
          }
          if ($include_root) $o .= '</' . $tag . '>';
        }
      }
      if ($is_data_block && $continue) {
        Frx::Data()->pop();
      }
    }
    else {

      // We can render so lets do it.
      $text = $node->asXML();
      $o .= $this->teng->replace($text);
    }

    // Restore link processing.
    if (@(string)$frx['invalid_link']) {
      $this->link_mode = $old_link_mode;
    }

    return $o;
  }

  /**
   * Render the report
   * @return unknown_type
   */
  public function render($format, $render_form=TRUE) {
    if (!$format) $format = 'web';
    // Only push the parameter conte
    Frx::Data()->push($this->parms, 'parm');

    $this->format = $format;
    $dom = $this->dom;
    $o='';
    $body = $dom->getElementsByTagName('body')->item(0);

    if (!$this->missing_parms || $this->rpt_xml->xpath('//*[@frx:renderer="FrxParameterForm"]')) foreach ($body->childNodes as $node) {
      $o .= $this->render_section($node);
    }
    // Default in dynamic title from head.
    if ($this->frx_title) {
      $title = check_plain($this->teng->replace($this->frx_title));
      if ($title) $title = $this->title = $title;
    }

    if ($render_form && !$this->rpt_xml->xpath('//*[@frx:renderer="FrxParameterForm"]') && $format=='web') {
      $variables = array('collapse' => $this->blocks_loaded);
      $form = $this->parametersForm($variables);
      $o = drupal_render($form) . $o;
    }
    $this->html = $o;
    Frx::Data()->pop();
    return $o ;
  }


  /**
   * Convert a relative link to appropriately rendered html
   * return html A properly formatted anchor tag
   */
  public function link($title, $path, $options=array()) {
    // check if we have
    $l = '';
    if (strpos($path, ':')===FALSE) {
      switch ($this->link_mode) {
        case 'remove':
          $l = '';
          break;
        case 'no-link':
        case 'text':
          $valid = drupal_valid_path($path,FALSE);
          $l = $valid ? l($title, $path, $options) : $title;
          break;
        case 'disable':
          $valid = drupal_valid_path($path,FALSE);
          if (!$valid) {
            $options['attributes']['class'][] = 'disabled';
            $l = '<a '. drupal_attributes($options['attributes']) . '>' . check_plain($title) . '</a>';
          }
          else {
            $l = l($title, $path, $options);
          }
          break;
        default:
          $l = l($title, $path, $options);
      }
    }
    else {
      $l = l($title, $path, $options);
    }
    return $l;
  }

  /*
   * Formatter used by the syntax engine to alter data that gets extracted.
   * This invokes the field translation
   */
  public function format($value, $key) {
    // Determine if there is a field overide entry
    $default='';
    $link ='';
    $format='';
    $format_str = '';
    $target = '';
    $class = '';
    $rel = '';
    if ($this->fields) {
      $path = 'frx:field[@id="' . $key . '"]';
      $formatters = $this->fields->xpath($path);
      if ($formatters) foreach ($formatters as $formatter) {
        if ((isset($formatter['block']) &&(string)$formatter['block'] == $this->block) || (!(string)$formatter['block'])) {

          //@TODO: Replace the default extraction with something that will get sub elements of the string
          $default = (string)$formatter;
          $link = (string)$formatter['link'];
          $add_query = (string)$formatter['add-query'];
          $class = (string)$formatter['class'];
          $rel = (string)$formatter['rel'];
          $format = (string) $formatter['format'];
          $format_str = (string) $formatter['format-string'];
          $target = (string) $formatter['target'];
        }
      }
    }

    if ($format) {
      $value = FrxReportGenerator::$instance->format_data($value, $format, $format_str, $this->teng);
    }

    if (is_array($value)) {
      $value = implode(' ', $value);
    }

    // Default if specified
    if (!$value && $default) {
      $value = $default;
    }

    if ($link) {
      $attributes = array();
      $target = $this->teng->replace($target, TRUE);
      // use the target attribute to open links in new tabs or as popups.
      if (@strpos(strtolower($target), 'popup')===0) {
        $opts = 'status=1';
        $options = "status=1";
        $attributes = array('onclick' =>
          'window.open(this.href, \'' . $target . '\', "' . $options . '"); return false;');
      }
      else {
        if ($target) $attributes['target'] = $target;
      }
      if ($rel) $attributes['rel'] = $this->teng->replace($rel, TRUE);
      if ($class) $attributes['class'] = explode(' ', trim($this->teng->replace($class, TRUE)));
      @list($url, $query) = explode('?', $link);
      $url = $this->teng->replace($url, TRUE);
      @list($query, $queryFrag) = explode('#', $query);
      @list($url, $fragment) = explode('#', $url);
      $fragment = $fragment . $queryFrag;
      $data = array();
      parse_str($query, $data);
      if ($data) foreach ($data as $k => $v) {
        $data[$k] = $this->teng->replace($v, TRUE);
      }
      if ($add_query) {
        $parms = $_GET;
        unset($parms['q']);
        $data = array_merge($parms, $data);
      }
      if (trim($url)) $value = $this->link(
        htmlspecialchars_decode($value),
        $url,
        array('fragment' => $fragment, 'query' => $data, 'attributes' => $attributes, 'absolute' => TRUE)
      );
    }
    return $value;
  }

  /**
   * Delete a node based on id
   * @param unknown_type $id
   * @return unknown_type
   */
  public function deleteNode($id) {
    $path = '//*[@id="' . $id . '"]';
    $nodes = $this->rpt_xml->xpath($path);
    if ($nodes) {
      $node = $nodes[0];
      $dom=dom_import_simplexml($node);
      $dom->parentNode->removeChild($dom);
    }
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
  /**
   * Make sure all xml elements have ids
   */
  private function parse_ids() {
    $i=0;
    if ($this->rpt_xml) {
      $this->rpt_xml->registerXPathNamespace('frx', FRX_NS);
      $frx_attributes = array();
      $frx_nodes = $this->rpt_xml->xpath('body//*[@frx:*]');

      if ($frx_nodes) foreach ($frx_nodes as $node) {
        $attr_nodes = $node->attributes(FRX_NS);
        if ($attr_nodes) {
          // Make sure every element has an id
          $i++;
          $id = 'forena-' . $i;

          if (!(string)$node['id']) {
            $node->addAttribute('id', $id);

          }
          else {
            if (strpos((string)$node['id'], 'forena-')===0) {
              // Reset the id to the numerically generated one
              $node['id'] = $id;
            }
            else {
              // Use the id of the element
              $id = (string)$node['id'];
            }
          }

          // Save away the frx attributes in case we need them later.
          $attr_nodes = $node->attributes(FRX_NS);
          $attrs = array();
          if ($attr_nodes) foreach ($attr_nodes as $key => $value) {
            $attrs[$key] = (string)$value;
          }
          // Save away the attributes
          $frx_attributes[$id] = $attrs;
        }
      }

      $this->frx_attributes = $frx_attributes;
    }
  }

  /**
   * Get the attributes by
   *
   * @return array Attributes
   *
   * This function will return an array for all of the frx attributes defined in the report body
   * These attributes can be saved away and added back in later using.
   */
  public function get_attributes_by_id() {
    $this->parse_ids();
    return $this->frx_attributes;
  }

  /**
   * Save attributes based on id match
   *
   * @param array $attributes
   *
   * The attributes array should be of the form
   * array( element_id => array( key1 => value1, key2 => value2)
   * The function restores the attributes based on the element id.
   */
  public function save_attributes_by_id($attributes) {
    $rpt_xml = $this->rpt_xml;
    if ($attributes) foreach ($attributes as $id => $att_list) {
      $id_search_path = '//*[@id="' . $id . '"]';
      $fnd = $rpt_xml->xpath($id_search_path);
      if ($fnd) {
        $node = $fnd[0];

        // Start attribute replacement
        $frx_attributes = $node->Attributes(FRX_NS);

        foreach ($att_list as $key => $value) {
          if (!$frx_attributes[$key]) {
            $node['frx:' . $key] = $value;
          }
          else {
            unset($frx_attributes[$key]);
            $node['frx:' . $key] = $value;
          }
        }
      }
    }
  }

  /**
   * Set the value of an element within the report
   * @param String $xpath Xpath to element being saved
   * @param string $value Value to be saved.
   * @return unknown_type
   */
  public function set_value($xpath, $value) {
    $xml = $this->rpt_xml;
    $i = strrpos($xpath, '/');
    $path = substr($xpath, 0, $i);
    $key = substr($xpath, $i+1);
    $nodes = $xml->xpath($path);
    if ($nodes) {
      // if the last part of the xpath is a key then assume the key
      if (strpos($key, '@')===0) {
        $key = trim($key, '@');
        if (is_null($value)) {
          unset($nodes[0][$key]);
        }
        else {
          $nodes[0][$key] = $value;
        }
      }
      // We must be refering to the text element of a node.
      else {
        if (is_null($value)) {
          unset($nodes[0]->$key);
        }
        else {
          $nodes[0]->$key = $value;
        }
      }
    }
  }

  /**
   * Default the parameters ba
   * @param $parms Array of parameters.
   * @return boolean indicating whether the required parameters are present.
   */
  public function processParameters() {
    $parms = $this->parms;
    $missing_parms = FALSE;

    foreach ($this->parameters as $key => $parm) {

      if ((@$parms[$key]==='' || @$parms[$key]===array() || @$parms[$key]===NULL) && @$parm['value']) {
        $value = $parm['value'];
        $options = array();
        if (@$parm['options']) {
          parse_str($parm['options'],$options);
        }
        switch ((string)@$parm['type']) {
          case 'date_text':
          case 'date_popup':
          case 'date_select':
             if ($value){
                $date_format = @$options['date_format'] ? $options['date_format'] : 'Y-m-d';
                $datetime = @strtotime($value);
                if ($datetime) {
                  $value = date($date_format, $datetime);
                }

              }
            break;
          default:
            if (strpos($value, '|')!==FALSE) {
              $value = explode('|', $value);
            }
        }
        $parms[$key] = $value;
        $reload_params = TRUE;
      }
      //do not show report if a required parameter does not have a value
      //force the user to input a parameter

      if ((@!$parms[$key]) && (@strcmp($parm['require'], "1") == 0)) {
        $missing_parms = TRUE;
      }
    }
    $this->parms = $parms;
    $this->missing_parms = $missing_parms;
    return $missing_parms;
  }

  public function parametersArray() {
    $parameters = array();
    $head = $this->rpt_xml->head;

    //FrxReportGenerator::instance()->alter_parameters('', $parms);
    $nodes = $head->xpath('frx:parameters/frx:parm');
    if ($nodes) {
      foreach ($nodes as $node) {
        $parm_def=array();
        $parm_def['default'] = (string)$node;
        foreach ($node->attributes() as $key => $value) {
          $parm_def[$key] = (string)$value;
        }
        $id = @$parm_def['id'];
        $parameters[$id] = $parm_def;
      }
    }
    return $parameters;
  }

  public function parametersForm($variables = array()) {

    $parms = $this->parametersArray();

    return drupal_get_form('forena_parameter_form', $parms, $variables);
  }

}


