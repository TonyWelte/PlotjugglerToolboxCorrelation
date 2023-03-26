#include "toolbox_correlation.h"
#include "ui_toolbox_correlation.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QMimeData>
#include <QDebug>
#include <QDragEnterEvent>
#include <QSettings>

#include "PlotJuggler/transform_function.h"
#include "PlotJuggler/svg_util.h"
#include "KissFFT/kiss_fftr.h"

ToolboxCorrelation::ToolboxCorrelation()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::toolbox_correlation;

  ui->setupUi(_widget);

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolboxPlugin::closed);

  connect(ui->pushButtonCalculate, &QPushButton::clicked, this,
          &ToolboxCorrelation::calculateCurveCorrelation);

  connect(ui->pushButtonSave, &QPushButton::clicked, this, &ToolboxCorrelation::onSaveCurve);

  connect(ui->pushButtonClear, &QPushButton::clicked, this, &ToolboxCorrelation::onClearCurves);
}

ToolboxCorrelation::~ToolboxCorrelation()
{
  delete ui;
}

void ToolboxCorrelation::init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

  _plot_widget_A = new PJ::PlotWidgetBase(ui->framePlotPreviewA);
  _plot_widget_B = new PJ::PlotWidgetBase(ui->framePlotPreviewB);
  _plot_widget_C = new PJ::PlotWidgetBase(ui->framePlotPreviewC);

  auto preview_layout_A = new QHBoxLayout(ui->framePlotPreviewA);
  preview_layout_A->setMargin(6);
  preview_layout_A->addWidget(_plot_widget_A);

  auto preview_layout_B = new QHBoxLayout(ui->framePlotPreviewB);
  preview_layout_B->setMargin(6);
  preview_layout_B->addWidget(_plot_widget_B);

  auto preview_layout_C = new QHBoxLayout(ui->framePlotPreviewC);
  preview_layout_C->setMargin(6);
  preview_layout_C->addWidget(_plot_widget_C);

  _plot_widget_A->setAcceptDrops(true);
  _plot_widget_B->setAcceptDrops(true);

  connect(_plot_widget_A, &PlotWidgetBase::dragEnterSignal, this,
          &ToolboxCorrelation::onDragEnterEventA);

  connect(_plot_widget_A, &PlotWidgetBase::dropSignal, this, &ToolboxCorrelation::onDropEventA);

  connect(_plot_widget_A, &PlotWidgetBase::viewResized, this, &ToolboxCorrelation::onViewResizedA);

  connect(_plot_widget_B, &PlotWidgetBase::dragEnterSignal, this,
          &ToolboxCorrelation::onDragEnterEventB);

  connect(_plot_widget_B, &PlotWidgetBase::dropSignal, this, &ToolboxCorrelation::onDropEventB);

  connect(_plot_widget_B, &PlotWidgetBase::viewResized, this, &ToolboxCorrelation::onViewResizedB);
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxCorrelation::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxCorrelation::onShowWidget()
{
  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();

  ui->pushButtonClear->setIcon(LoadSvg(":/resources/svg/clear.svg", theme));
  return true;
}

std::vector<kiss_fft_scalar> plotdata_to_vector(PlotData& data) {
  std::vector<kiss_fft_scalar> output;
  output.reserve(data.size());
  for(const auto& d : data) {
    output.push_back(d.y);
  }
  return output;
}

void ToolboxCorrelation::calculateCurveCorrelation()
{
  _plot_widget_C->removeAllCurves();

  std::vector<kiss_fft_scalar> base;

  if(_curve_names_A.empty()) {
    return;
  }

  base = plotdata_to_vector(_plot_data->numeric.find(_curve_names_A.front())->second);
  

  for (const auto& curve_id : _curve_names_B)
  {
    auto it = _plot_data->numeric.find(curve_id);
    if (it == _plot_data->numeric.end())
    {
      return;
    }
    PlotData& curve_data = it->second;

    std::vector<kiss_fft_scalar> second = plotdata_to_vector(curve_data);

    size_t N = second.size();
    N *= 2;

    std::vector<kiss_fft_scalar> first(base);


    first.resize(N, (kiss_fft_scalar)0.0);
    second.resize(N, (kiss_fft_scalar)0.0);
    
    std::vector<kiss_fft_cpx> first_fft(N/2+1);
    std::vector<kiss_fft_cpx> second_fft(N/2+1);
    std::vector<kiss_fft_cpx> result_fft(N/2+1);
    std::vector<kiss_fft_scalar> result(N);

    auto config = kiss_fftr_alloc(N, false, nullptr, nullptr);
    auto configi = kiss_fftr_alloc(N, true, nullptr, nullptr);
    
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    std::cout << first.size() << std::endl << second.size() << std::endl;

    kiss_fftr(config, first.data(), first_fft.data());
    kiss_fftr(config, second.data(), second_fft.data());
    for(int i = 0; i < N/2+1; ++i) {
      result_fft[i].r = first_fft[i].r * second_fft[i].r + first_fft[i].i * second_fft[i].i;
      result_fft[i].i = - first_fft[i].r * second_fft[i].i + first_fft[i].i * second_fft[i].r;
    }
    kiss_fftri(configi, result_fft.data(), result.data());

    QApplication::restoreOverrideCursor();

    std::cout << result.size() << std::endl;

    auto& curver_correlate = _local_data.getOrCreateNumeric(curve_id);
    curver_correlate.clear();
    for (int i = N/2; i < result.size()+(int)N/2; i++)
    {
      curver_correlate.pushBack({ i-(int)N, (float)result.at(i%N) });
    }
    std::cout <<std::endl;
    QColor color = Qt::transparent;
    auto colorHint = curve_data.attribute(COLOR_HINT);
    if (colorHint.isValid())
    {
      color = colorHint.value<QColor>();
    }

    _plot_widget_C->addCurve(curve_id + "_correlation", curver_correlate, color);

    free(config);
  }

  _plot_widget_C->resetZoom();
}

void ToolboxCorrelation::onClearCurves()
{
  _plot_widget_A->removeAllCurves();
  _plot_widget_A->resetZoom();

  _plot_widget_B->removeAllCurves();
  _plot_widget_B->resetZoom();

  ui->pushButtonSave->setEnabled(false);
  ui->pushButtonCalculate->setEnabled(false);

  ui->lineEditSuffix->setEnabled(false);
  ui->lineEditSuffix->setText("_cor");

  _curve_names_A.clear();
}

void ToolboxCorrelation::onDragEnterEventA(QDragEnterEvent* event)
{
  const QMimeData* mimeData = event->mimeData();
  QStringList mimeFormats = mimeData->formats();

  for (const QString& format : mimeFormats)
  {
    QByteArray encoded = mimeData->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (format != "curveslist/add_curve")
    {
      return;
    }

    QStringList curves;
    while (!stream.atEnd())
    {
      QString curve_name;
      stream >> curve_name;
      if (!curve_name.isEmpty())
      {
        curves.push_back(curve_name);
      }
    }
    _dragging_curves = curves;
    event->accept();
  }
}

void ToolboxCorrelation::onDropEventA(QDropEvent*)
{
  _zoom_range.min = std::numeric_limits<double>::lowest();
  _zoom_range.max = std::numeric_limits<double>::max();

  for (auto& curve : _dragging_curves)
  {
    std::string curve_id = curve.toStdString();
    PlotData& curve_data = _plot_data->getOrCreateNumeric(curve_id);

    _plot_widget_A->addCurve(curve_id, curve_data);
    _curve_names_A.push_back(curve_id);
    _zoom_range.min = std::min(_zoom_range.min, curve_data.front().x);
    _zoom_range.max = std::max(_zoom_range.max, curve_data.back().x);
  }

  ui->pushButtonSave->setEnabled(true);
  ui->pushButtonCalculate->setEnabled(true);
  ui->lineEditSuffix->setEnabled(true);

  _dragging_curves.clear();
  _plot_widget_A->resetZoom();
}

void ToolboxCorrelation::onViewResizedA(const QRectF& rect)
{
  _zoom_range.min = rect.left();
  _zoom_range.max = rect.right();
}

void ToolboxCorrelation::onDragEnterEventB(QDragEnterEvent* event)
{
  const QMimeData* mimeData = event->mimeData();
  QStringList mimeFormats = mimeData->formats();

  for (const QString& format : mimeFormats)
  {
    QByteArray encoded = mimeData->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (format != "curveslist/add_curve")
    {
      return;
    }

    QStringList curves;
    while (!stream.atEnd())
    {
      QString curve_name;
      stream >> curve_name;
      if (!curve_name.isEmpty())
      {
        curves.push_back(curve_name);
      }
    }
    _dragging_curves = curves;
    event->accept();
  }
}

void ToolboxCorrelation::onDropEventB(QDropEvent*)
{
  // _zoom_range.min = std::numeric_limits<double>::lowest();
  // _zoom_range.max = std::numeric_limits<double>::max();

  for (auto& curve : _dragging_curves)
  {
    std::string curve_id = curve.toStdString();
    PlotData& curve_data = _plot_data->getOrCreateNumeric(curve_id);

    _plot_widget_B->addCurve(curve_id, curve_data);
    _curve_names_B.push_back(curve_id);
    // _zoom_range.min = std::min(_zoom_range.min, curve_data.front().x);
    // _zoom_range.max = std::max(_zoom_range.max, curve_data.back().x);
  }

  ui->pushButtonSave->setEnabled(true);
  ui->pushButtonCalculate->setEnabled(true);
  ui->lineEditSuffix->setEnabled(true);

  _dragging_curves.clear();
  _plot_widget_B->resetZoom();
}

void ToolboxCorrelation::onViewResizedB(const QRectF& rect)
{
  // _zoom_range.min = rect.left();
  // _zoom_range.max = rect.right();
}

void ToolboxCorrelation::onSaveCurve()
{
  auto suffix = ui->lineEditSuffix->text().toStdString();
  if (suffix.empty())
  {
    ui->lineEditSuffix->setText("_cor");
    suffix = "_cor";
  }
  for (const auto& curve_id : _curve_names_B)
  {
    auto it = _local_data.numeric.find(curve_id);
    if (it == _local_data.numeric.end())
    {
      continue;
    }
    auto& out_data = _plot_data->getOrCreateNumeric(curve_id + suffix);
    out_data.clonePoints(it->second);

    // TODO out_data.setAttribute(PJ::DISABLE_LINKED_ZOOM, true);
    emit plotCreated(curve_id + suffix);
  }

  emit closed();
}
