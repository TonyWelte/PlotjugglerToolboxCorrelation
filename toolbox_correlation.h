#pragma once

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"

namespace Ui
{
class toolbox_correlation;
}

class ToolboxCorrelation : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxCorrelation();

  ~ToolboxCorrelation() override;

  const char* name() const override
  {
    return "Correlation/Autocorrelation";
  }

  void init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map) override;

  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:

  bool onShowWidget() override;

private:
  QWidget* _widget;
  Ui::toolbox_correlation* ui;

  // bool eventFilter(QObject *obj, QEvent *event) override;

  QStringList _dragging_curves;

  PJ::PlotWidgetBase* _plot_widget_A = nullptr;
  PJ::PlotWidgetBase* _plot_widget_B = nullptr;
  PJ::PlotWidgetBase* _plot_widget_C = nullptr;

  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;

  PJ::PlotDataMapRef _local_data;

  Range _zoom_range;

  std::vector<std::string> _curve_names_A;
  std::vector<std::string> _curve_names_B;

private slots:

  void onDragEnterEventA(QDragEnterEvent* event);
  void onDropEventA(QDropEvent* event);
  void onViewResizedA(const QRectF& rect);
  void onDragEnterEventB(QDragEnterEvent* event);
  void onDropEventB(QDropEvent* event);
  void onViewResizedB(const QRectF& rect);
  void onSaveCurve();
  void calculateCurveCorrelation();
  void onClearCurves();
};
