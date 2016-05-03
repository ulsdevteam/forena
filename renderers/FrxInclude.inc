<?php
/**
 * @file FrxInclude
 * Include a reference to another report as an asset.
 * @author davidmetzler
 *
 */
class FrxInclude extends FrxRenderer {
  public function render() {
    // Get data from source
    $attributes = $this->mergedAttributes();
    $output = '';

    // Determine data type
    $include = @$attributes['src'];
    $title = @$attributes['title'];
    // Quit if we have no data.
    if (!$include) return '';

    // Reformat URL
    @list($url, $query_str)=@explode('?', $include);
    $url = $this->teng->replace($url, TRUE);
    $report_url = $url;
    $parts = @explode('/', $url);
    $file = @$parts[count($parts) - 1];
    $parts = explode('.', $file);
    // Determine file extention
    $ext = count($parts) > 1 ? $parts[count($parts) - 1] : '';
    $query = array();
    if ($query_str) {
      parse_str($query_str, $query );
      foreach ($query as $key=>$value) {
        $query[$key] = $this->teng->replace($value, TRUE);
      }
    }

    // Build URL
    $options = array('query' => $query);
    $url = url($url, $options);

    $mode = isset($attributes['mode']) ? $attributes['mode'] : '';
    if (!$mode) {
      // Try and default in old method for loading.
      if ($this->format == 'web' && strlen($ext)<=4) {
        $mode = 'reference';
      }
    }

    switch ($mode) {
      case 'ajax':
        if(strpos($url,'/nojs/')=== FALSE) {
          if (!isset($attributes['id'])) $attributes['id'] = 'frx-include';
          $id = @$attributes['id'];
          $url = url("$report_url/nojs/$id/replace", $options);
          if (isset($attributes['class'])) {
            $attributes['class'] .= ' use-ajax forena-autoload';
          }
          else {
            $attributes['class'] = 'use-ajax forena-autoload';
          }
        }
        $output = $this->render_reference($url, $ext, $attributes, $title);
        break;
      case 'reference':
        $output = $this->render_reference($url, $ext, $attributes, $title);
        break;
      case 'inline':
      default:
        $output = forena_report_include(str_replace('reports/', '', $report_url), $query);

    }
    return $output;
  }

  function render_reference($url, $ext, $attributes, $title) {
    $ext = strtolower($ext);
    if (!$title) $title = "$ext document";
    $attributes = $this->teng->replace($attributes);
    switch ($ext) {
      case 'png':
      case 'gif':
      case 'jpg':
      case 'jpeg':
        $x = new SimpleXMLElement('<img/>');
        $x['src'] = $url;
        if (isset($attributes['height'])) $x['height'] = $attributes['height'];
        if (isset($attributes['width'])) $x['width'] = $attributes['width'];
        break;
      case 'svg':
        $x = new SimpleXMLElement('<embed/>');
        $x['src'] = $url;
        $x['type'] = 'image/svg+xml';
        $x['pluginspage'] = "http://www.adobe.com/svg/viewer/install/";
        if (isset($attributes['height'])) $x['height'] = $attributes['height'];
        if (isset($attributes['width'])) $x['width'] = $attributes['width'];
        break;
      default:
        $x = new SimpleXMLElement('<a>' . htmlentities($title, ENT_QUOTES) . '</a>' );
        $x['href'] = $url;
    }

    if (isset($attributes['id'])) $x['id'] = $attributes['id'];
    if (isset($attributes['class'])) $x['class'] = $attributes['class'];
    return $x->asXML();
  }

  /**
   * Implement template generator.
   * @see FrxRenderer::generate()
   */
  public function generate($xml, &$config) {
    $src = $this->extract('src', $key);
    $div = $this->blockDiv($config);
    $frx = array('src' => $src, 'renderer' => 'FrxInclude');
    $this->addNode($div, 4, 'div', NULL, $config, $frx);
  }

}