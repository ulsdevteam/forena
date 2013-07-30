<?php
/**
 * @file FrxChart
 * Base class for chart generation implementations.
 * @author davidmetzler
 *
 */
class FrxChart extends FrxRenderer {
  public $xy_data = FALSE;
  public function render() {

    // Get data from source
    $output = '';
    $options = array();
    $series = array();
    $seriesx = array();
    $data = array();
    $attributes = $this->mergedAttributes();
    $path = isset($attributes['xpath']) ? $attributes['xpath'] :  '*';
    $label = '';
    $links = '';
    $type = $attributes['type'];
    $link = @$attributes['link'];
    // Legacy options.  New charts should be generated using Frx:attribute syntax
    if (isset($attributes['options'])) {
      parse_str($attributes['options'], $options);
      unset($attributes['options']);
      $options  = array_merge($attributes, $options);
    }
    else {
      $options = $attributes;
    }

    $series = @$options['series'];
    if (isset($options['seriesx']))  {
      $seriesx = $options['seriesx'];
      unset($options['seriesx']);
    }
    if (isset($options['label'])) {
      $label =  $options['label'];
      unset($options['label']);
    }
    foreach ($options as $key => $value) {
      $options[$key] = $this->teng->replace($value, TRUE);
    }

    if (isset($attributes['height'])) $options['height'] = $this->teng->replace($attributes['height']);
    if (isset($attributes['width'])) $options['width'] = $this->teng->replace($attributes['width']);

    // Extract chart series data from xml.
    if ($series) {
      foreach ((array)$series as $i => $d) {
        if ($seriesx) {
          $this->xy_data = TRUE;
          $data[] = $this->xmlToValues($path, $d, $seriesx[$i], TRUE);
        }
        else {
          $data[] = $this->xmlToValues($path, $d, $label, FALSE);
        }
      }
    }

    if ($link) {
      $links = $this->xmlToValues($path, $link, $label);
    }

    // Deal with rare case where attributes are supposed to come from data
    if (@$attributes['color'] && !is_array($attributes['color']) && strpos($attributes['color'], '{') !==FALSE) {
      $options['colors'] = $this->xmlToValues($path, $attributes['color']);
    }

    if ($data) {

      if (method_exists($this, 'renderChart')) {
        $output = $this->renderChart($type, $data, $options, $links);
      }

    }
    return $output;
  }


  public function xmlToValues($path, $data_path, $label_path='', $pairs = FALSE) {
    $do = Frx::Data();
    $data = $do->currentContext();
    $values = array();
    if (is_object($data)) {
      $nodes = $data->xpath($path);
      if ($nodes) foreach ($nodes as $i => $node) {
        $do->push($node, $this->id);

        $val = $this->teng->replace($data_path, TRUE);
        if ($label_path) {
          $key = strip_tags($this->teng->replace($label_path, FALSE));
        }
        else {
          $key = $i;
        }
        if ($pairs && $label_path) {
          $values[] = array(floatval($key), floatval($val));
        }
        else {
          $values[$key] = $val;
        }


        $do->pop();
      }
    }
    return $values;
  }
}