#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "CloudProviderModel.h"
#include "NavigationModel.h"

using namespace Qt::StringLiterals;

class TestCloudProviderModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void testInitialModelPopulation();
    void testActiveProviderProperties();
    void testProviderLookupAndIndex();
    void testNavigationModelIsCloud();
};

void TestCloudProviderModel::initTestCase() {
    QCoreApplication::setOrganizationName(u"QTranscribeTestOrg"_s);
    QCoreApplication::setApplicationName(u"QTranscribeTestApp"_s);
}

void TestCloudProviderModel::cleanup() {
    QSettings settings;
    settings.remove(u"Cloud/ActiveProvider"_s);
    settings.sync();
}

void TestCloudProviderModel::testInitialModelPopulation() {
    CloudProviderModel model;
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);

    const QModelIndex firstIdx = model.index(0, 0);
    QVERIFY(firstIdx.isValid());

    const QString providerId = model.data(firstIdx, CloudProviderModel::IdRole).toString();
    QCOMPARE(providerId, u"groq"_s);

    const QString name = model.data(firstIdx, CloudProviderModel::NameRole).toString();
    QCOMPARE(name, tr("Groq Cloud"));

    const QString desc = model.data(firstIdx, CloudProviderModel::DescriptionRole).toString();
    QVERIFY(!desc.isEmpty());

    const QString componentSource = model.data(firstIdx, CloudProviderModel::ComponentSourceRole).toString();
    QCOMPARE(componentSource, u"providers/GroqProviderSettings.qml"_s);

    const QString websiteUrl = model.data(firstIdx, CloudProviderModel::WebsiteUrlRole).toString();
    QCOMPARE(websiteUrl, u"https://console.groq.com/keys"_s);

    const bool isActive = model.data(firstIdx, CloudProviderModel::IsActiveRole).toBool();
    QVERIFY(isActive);

    const QModelIndex secondIdx = model.index(1, 0);
    QVERIFY(secondIdx.isValid());
    QCOMPARE(model.data(secondIdx, CloudProviderModel::IdRole).toString(), u"gemini"_s);
    QCOMPARE(model.data(secondIdx, CloudProviderModel::NameRole).toString(), tr("Google Gemini"));
    QCOMPARE(model.data(secondIdx, CloudProviderModel::ComponentSourceRole).toString(), u"providers/GeminiProviderSettings.qml"_s);
    QCOMPARE(model.data(secondIdx, CloudProviderModel::WebsiteUrlRole).toString(), u"https://aistudio.google.com/app/apikey"_s);
}

void TestCloudProviderModel::testActiveProviderProperties() {
    CloudProviderModel model;
    QCOMPARE(model.activeProviderId(), u"groq"_s);
    QCOMPARE(model.activeProviderIndex(), 0);
    QCOMPARE(model.activeProviderName(), tr("Groq Cloud"));
    QCOMPARE(model.activeProviderComponent(), u"providers/GroqProviderSettings.qml"_s);
    QCOMPARE(model.activeProviderWebsiteUrl(), u"https://console.groq.com/keys"_s);
    QVERIFY(!model.activeProviderDescription().isEmpty());
}

void TestCloudProviderModel::testProviderLookupAndIndex() {
    CloudProviderModel model;
    QCOMPARE(model.indexOfProvider(u"groq"_s), 0);
    QCOMPARE(model.indexOfProvider(u"gemini"_s), 1);
    QCOMPARE(model.indexOfProvider(u"non_existent"_s), -1);
    QCOMPARE(model.providerIdAt(0), u"groq"_s);
    QCOMPARE(model.providerIdAt(1), u"gemini"_s);
    QCOMPARE(model.providerIdAt(99), QString());

    QSignalSpy activeChangedSpy(&model, &CloudProviderModel::activeProviderChanged);
    model.setActiveProviderIndex(0);
    QCOMPARE(activeChangedSpy.count(), 0);

    model.setActiveProviderIndex(1);
    QCOMPARE(activeChangedSpy.count(), 1);
    QCOMPARE(model.activeProviderId(), u"gemini"_s);
    QCOMPARE(model.activeProviderName(), tr("Google Gemini"));

    model.setActiveProviderId(u"non_existent"_s);
    QCOMPARE(activeChangedSpy.count(), 1);
    QCOMPARE(model.activeProviderId(), u"gemini"_s);

    model.setActiveProviderId(u"groq"_s);
    QCOMPARE(activeChangedSpy.count(), 2);
    QCOMPARE(model.activeProviderId(), u"groq"_s);
}

void TestCloudProviderModel::testNavigationModelIsCloud() {
    NavigationModel navModel;
    QCOMPARE(navModel.isCloud(), false);
    QCOMPARE(navModel.isOnline(), false);

    QSignalSpy isCloudSpy(&navModel, &NavigationModel::isCloudChanged);
    navModel.setIsCloud(true);

    QCOMPARE(isCloudSpy.count(), 1);
    QCOMPARE(navModel.isCloud(), true);
    QCOMPARE(navModel.isOnline(), true);

    const int count = navModel.rowCount();
    QVERIFY(count > 0);

    bool foundDictationSection = false;
    for (int i = 0; i < count; ++i) {
        const QModelIndex idx = navModel.index(i, 0);
        const QString sectionId = navModel.data(idx, NavigationModel::SectionIdRole).toString();
        const QString title = navModel.data(idx, NavigationModel::TitleRole).toString();
        if (title == tr("Dictation")) {
            foundDictationSection = true;
            QCOMPARE(sectionId, u"cloud"_s);
        }
    }
    QVERIFY(foundDictationSection);

    navModel.setIsCloud(false);
    for (int i = 0; i < count; ++i) {
        const QModelIndex idx = navModel.index(i, 0);
        const QString sectionId = navModel.data(idx, NavigationModel::SectionIdRole).toString();
        const QString title = navModel.data(idx, NavigationModel::TitleRole).toString();
        if (title == tr("Dictation")) {
            QCOMPARE(sectionId, u"offline"_s);
        }
    }
}

QTEST_GUILESS_MAIN(TestCloudProviderModel)
#include "test_cloud_provider_model.moc"
