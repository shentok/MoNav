#include "chsettingsdialog.h"
#include "ui_chsettingsdialog.h"
#include <QSettings>
#include <omp.h>

CHSettingsDialog::CHSettingsDialog(QWidget *parent) :
	 QWidget(parent),
	 m_ui(new Ui::CHSettingsDialog)
{
	 m_ui->setupUi(this);

	QSettings settings( "MoNav" );
	settings.beginGroup( "Contraction Hierarchies" );
	m_ui->blockSize->setValue( settings.value( "blockSize", 12 ).toInt() );
}

CHSettingsDialog::~CHSettingsDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Contraction Hierarchies" );
	settings.setValue( "blockSize", m_ui->blockSize->value() );

	 delete m_ui;
}

bool CHSettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->blockSize = m_ui->blockSize->value();
	return true;
}
