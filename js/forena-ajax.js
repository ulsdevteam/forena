/**
 * @file
 *
 * Forena ajax library.  Includes useful functions for performing ajax replacements on reports.
 */
(function ($) {

  $.fn.forenaAutoload = function () {
    this.hide();
    this.addClass(".forena-autoload-processed");
    this.click();
  }

  Drupal.behaviors.forenaAjax = {
    attach: function (context, settings) {
      // Auto click the reports for ajax.
      $('.use-ajax.ajax-processed.forena-autoload:not(.forena-autoload-processed', context).forenaAutoload();
    }
  };

})(jQuery);

