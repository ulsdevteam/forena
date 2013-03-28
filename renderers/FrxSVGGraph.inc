<?php
/**
 * @file FrxSVGGraph
 * php SVG Graph generator
 *
 * @author davidmetzler
 *
 */
require_once ('FrxChart.inc');
class FrxSVGGraph extends FrxChart {
  private $graph;
  private $links;
  function __construct() {
    $directory = FrxReportGenerator::instance()->configuration('library_path');
    $library = rtrim($directory, '/') . '/SVGGraph/SVGGraph.php';
    require_once $library;
  }

  function renderChart($type, $data, $options, $links ) {
    $options['width'] = @$options['width'] ? $options['width'] : 600;
    $options['height'] = @$options['height'] ? $options['height'] : 400;
    $this->links = $links;
    if ($data) {
      switch ($type) {
        case 'bargraph':
          $output = $this->renderGraph('BarGraph', $options, $data[0]);
          break;
        case 'bar3dgraph':
          $output = $this->renderGraph('Bar3DGraph', $options, $data[0]);
          break;
        case 'horizontalbargraph':
          $output = $this->renderGraph('HorizontalBarGraph', $options, $data[0]);
        case 'piechart':
          $output = $this->pieChart($data[0], $options);
          break;
        case 'stackedbargraph':
          $output = $this->stackedBarGraph($data, $options);
          break;
        case 'groupedbargraph':
          $output = $this->groupedBarGraph($data, $options);
          break;
        case 'linegraph':
          $output = $this->renderGraph('LineGraph', $options, $data[0]);
          break;
        case 'multilinegraph':
          $output = $this->renderGraph('MultiLineGraph', $options, $data);
          break;
        case 'scatterplot':
          $output = $this->renderGraph('ScatterGraph', $options, $data);
          break;
        case 'multiscatterplot':
          $output = $this->renderGraph('MultiScatterGraph', $options, $data);
          break;
        case 'cylindergraph':
          $output = $this->renderGraph('CylinderGraph', $options, $data[0]);
          break;
        case 'stackedcylindergraph':
          $output = $this->renderGraph('StackedCylinderGraph', $options, $data);
          break;
        case 'groupedcylindergraph':
          $output = $this->renderGraph('GroupedCylinderGraph', $options, $data);
          break;
        case 'radargraph':
          $output = $this->renderGraph('RadarGraph', $options, $data[0]);
          break;
        case 'multiradargraph':
          $output = $this->renderGraph('MultiRadarGraph', $options, $data);
          break;

      }
    }
    return $output;
  }

  // Generate a bar graph.
  function barGraph($data, $options) {
    $type = $options['type'];
    switch(@$options['type']) {
      case '3D':
        $type = 'Bar3DGraph';
        break;
      case 'horizontal':
        $type = 'HorizontalBarGraph';
        break;
      case 'cylinder':
        $type = 'Cylinder';

      default:
        $type = 'BarGraph';
    }
    return $this->renderGraph($type, $options, $data);
  }

  function stackedBarGraph($data, $options) {
    $type = $options[$type];
    switch (@$options['style']) {
      case 'horizontal':
        $type = 'HorizontalStackedBarGraph';
        break;
      case 'cylinder':
        $type = 'StackedCylinder';
        break;
      default:
        $type='StackedBarGraph';
    }
    return $this->renderGraph($type, $options, $data);
  }

  function groupedBarGraph($data, $options) {
    switch (@$options['style']) {
      case 'horizontal':
        $type = 'HorizontalGroupedBarGraph';
        break;
      case 'groupedcylinder':
        $type = 'GroupCylinderGraph';
        break;
      default:
        $type = 'GroupedBarGraph';
    }
    return $this->renderGraph($type, $options, $data);
  }

  // Generate a pie chart
  function pieChart($data, $options) {
    if (@$options['style']=='3D') {
      $type = 'Pie3DGraph';
    }
    else {
      $type = 'PieGraph';
    }
    return $this->renderGraph($type, $options, $data);
  }

  function renderGraph($type, $options, $data) {
    if (!isset($options['scatter_2d']) && ($type == 'ScatterGraph' || $type=='MultiScatterGraph') && $this->xy_data && !isset($options['scatter_2d'])) {
      $options['scatter_2d'] = TRUE;
    }
    else {
      $options['scatter_2d'] = (bool) @$options['scatter_2d'];
    }
    $links = $this->links;
    $width = (@$options['width']) ? @$options['width'] : 600;
    $height = (@$options['height']) ? @$options['height'] : 400;
    $graph = new SVGGraph($width, $height, $options);
    $this->graph = $graph;
    $graph->Values($data);
    if (is_array(@$options['colors'])) {
      $graph->colours = $options['colors'];
    }

    if ($links) {
      $graph->Links($links);
    }
    $output = $graph->Fetch($type, FALSE);
    // Add a viewbox to be compatible with Prince PDF generation.
    $output = str_replace('<svg width', "<svg viewBox='0 0 $width $height' width", $output);
    $javascript = '/<script[^>]*?>.*?<\/script>/si';
    $noscript = '';
    if ($this->format != 'web') {
      $output = preg_replace($javascript, $noscript, $output);
    }
    return $output;
  }


}