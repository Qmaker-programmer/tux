#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QPoint>
#include <QTimer>
#include <QMouseEvent>
#include <QStringList>
#include <QRandomGenerator>
#include <QScreen>
#include <QGuiApplication>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
//  -- Variables Globbales --
const bool speak = true;
const bool rmove = true; 
const int sizeTux = 320;
const int initPosX = 400;
const int initPosY = 300;
const int margenSalto = 22; // espacio extra arriba para que el salto no se corte
//--         |             --
class VentanaFlotante : public QWidget {
    Q_OBJECT
    bool mirandoIzquierda = true; // El tux.png original en realidad mira hacia la IZQUIERDA
public:
    VentanaFlotante(QWidget *parent = nullptr) : QWidget(parent) {
        // 1. Configuración de la ventana principal (Tux)
        setWindowFlags(Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::SubWindow);
        setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground, true);

        // Cargar y escalar a Tux
        labelTux = new QLabel(this);
        QPixmap pixmapOriginal("tux.png");
        pixmap = pixmapOriginal.scaled(sizeTux, sizeTux, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
        labelTux->setPixmap(pixmap);
        // La ventana es más alta que la imagen: el sobrante de arriba es el "aire" para saltar
        resize(pixmap.width(), pixmap.height() + margenSalto);
        labelTux->move(0, margenSalto);

        // Posición inicial (Centro-derecha de la pantalla aprox)
        move(initPosX, initPosY);
        posicionPrevia = QPoint();

        // 2. Configuración del Bocadillo de Texto (Globo de diálogo)
        bocadillo = new QLabel(nullptr); // Se usa nullptr para que pueda tener sus propios flags de ventana
        bocadillo->setWindowFlags(Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::ToolTip);

        // ELIMINADO: bocadillo->setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground, true);

        // Estilo CSS: Cuadrado gris clásico
        bocadillo->setStyleSheet(
            "QLabel {"
            "    background-color: #D4D0C8;       /* Gris clásico de interfaz retro */"
            "    color: #000000;                  /* Texto negro puro */"
            "    border-top: 2px solid #FFFFFF;   /* Efecto 3D clásico: luz arriba e izquierda */"
            "    border-left: 2px solid #FFFFFF;"
            "    border-right: 2px solid #808080; /* Sombra abajo y derecha */"
            "    border-bottom: 2px solid #808080;"
            "    border-radius: 0px;              /* Totalmente cuadrado */"
            "    padding: 8px 12px;               /* Margen interno para el texto */"
            "    font-family: 'MS Sans Serif', 'Arial', sans-serif;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "}"
        );
        bocadillo->hide(); // Empieza oculto

        // Lista de mensajes que Tux dirá al azar
        mensajes << "¡Recuerda tomar agua!"
                 << "Parpadea un poco, descansa los ojos"
                 << "¡Estira la espalda!"
                 << "A ver... ¿qué estamos programando hoy?"
                 << "¡Todo marcha sobre ruedas!"
                 << "No te rindas con ese bug";

        // 3. TEMPORIZADOR 1: Cada 8 segundos decide si INICIAR una nueva caminata
        timerMovimiento = new QTimer(this);
        connect(timerMovimiento, &QTimer::timeout, this, &VentanaFlotante::decidirCaminata);
        timerMovimiento->start(8000); 

        // 3b. TEMPORIZADOR DE PASOS: da cada salto de la caminata en curso
        timerCaminata = new QTimer(this);
        connect(timerCaminata, &QTimer::timeout, this, &VentanaFlotante::darPaso);

        timerMensajes = new QTimer(this);
        connect(timerMensajes, &QTimer::timeout, this, &VentanaFlotante::hablar);
        timerMensajes->start(10000); 
        timerOcultar = new QTimer(this);
        timerOcultar->setSingleShot(true);
        connect(timerOcultar, &QTimer::timeout, bocadillo, &QLabel::hide);

        // 4. TEMPORIZADOR DE ANIMACIÓN (squash & stretch al caminar)
        timerAnimacion = new QTimer(this);
        connect(timerAnimacion, &QTimer::timeout, this, &VentanaFlotante::animarPaso);
    }


    ~VentanaFlotante() {
        delete bocadillo;
    }

protected:

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::MouseButton::LeftButton) {
            posicionPrevia = event->globalPosition().toPoint();
            event->accept();
        } else if (event->button() == Qt::MouseButton::RightButton) {
            exit(0);
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::MouseButton::LeftButton) {
            QPoint posActual = event->globalPosition().toPoint();
            QPoint delta = posActual - posicionPrevia;
            move(this->pos() + delta);
            posicionPrevia = posActual;

            if (bocadillo->isVisible()) {
                actualizarPosicionBocadillo();
            }
            event->accept();
        }
    }

private slots:

    void decidirCaminata() {
        if (!rmove) return;
        if (QApplication::mouseButtons() & Qt::MouseButton::LeftButton) return;
        if (timerCaminata->isActive()) return; // ya está en medio de una caminata, no la interrumpimos

        // Dirección aleatoria (cualquier ángulo) y distancia TOTAL aleatoria del recorrido
        double angulo = QRandomGenerator::global()->bounded(360) * M_PI / 180.0;
        int distanciaTotal = QRandomGenerator::global()->bounded(80, 481); // entre 80 y 480 px
        const int pasoPx = 28; // "largo de zancada" de cada salto

        pasosRestantes = std::max(1, distanciaTotal / pasoPx);
        direccionPaso = QPoint(int(std::cos(angulo) * pasoPx), int(std::sin(angulo) * pasoPx));

        timerCaminata->start(180); // tiempo entre cada salto del recorrido
    }

    void darPaso() {
        if (pasosRestantes <= 0 || (QApplication::mouseButtons() & Qt::MouseButton::LeftButton)) {
            timerCaminata->stop();
            return;
        }

        QPoint posActual = this->pos();
        QPoint nuevaPos = posActual + direccionPaso;

        // No dejar que se salga de la pantalla: recortamos contra los bordes
        QRect pantalla = QGuiApplication::primaryScreen()->availableGeometry();
        nuevaPos.setX(std::clamp(nuevaPos.x(), pantalla.left(), pantalla.right() - width()));
        nuevaPos.setY(std::clamp(nuevaPos.y(), pantalla.top(), pantalla.bottom() - height()));

        // Si chocó con un borde, cortamos el recorrido acá
        if (nuevaPos == posActual) {
            pasosRestantes = 0;
            timerCaminata->stop();
            return;
        }

        // Voltear el sprite según hacia dónde avanza horizontalmente
        if (direccionPaso.x() > 0 && mirandoIzquierda) {
            pixmap = pixmap.transformed(QTransform().scale(-1, 1));
            labelTux->setPixmap(pixmap);
            mirandoIzquierda = false;
        } else if (direccionPaso.x() < 0 && !mirandoIzquierda) {
            pixmap = pixmap.transformed(QTransform().scale(-1, 1));
            labelTux->setPixmap(pixmap);
            mirandoIzquierda = true;
        }

        move(nuevaPos);

        if (bocadillo->isVisible()) {
            actualizarPosicionBocadillo();
        }

        // Reproducir el salto (squash & stretch suave) en este paso del recorrido
        pasoAnimacion = 0;
        timerAnimacion->start(45);

        pasosRestantes--;
    }

    void animarPaso() {
        // Ciclo suave: apenas se achica/estira y sube un poco, sin sacudones
        static const double escalas[]  = {0.97, 1.02, 1.00, 0.98, 1.00};
        static const int    offsetsY[] = {0,    -8,   -12,  -5,   0};
        const int totalFrames = 5;

        if (pasoAnimacion >= totalFrames) {
            timerAnimacion->stop();
            labelTux->setPixmap(pixmap);   // Volver al tamaño y posición normal
            labelTux->move(0, margenSalto);
            return;
        }

        double escala = escalas[pasoAnimacion];
        int offsetY = offsetsY[pasoAnimacion];
        int nuevoAlto = int(pixmap.height() * escala);
        QPixmap frame = pixmap.scaled(pixmap.width(), nuevoAlto,
                                       Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        labelTux->setPixmap(frame);

        // Anclamos los "pies" abajo (dentro del margen) y sumamos el offset del salto
        int yBase = margenSalto + (pixmap.height() - nuevoAlto);
        labelTux->move(0, yBase + offsetY);

        pasoAnimacion++;
    }

    void hablar() {
        if (speak) {
            if (mensajes.isEmpty()) return;
            
            int index = QRandomGenerator::global()->bounded(mensajes.size());
            QString texto = mensajes.at(index);
            
            bocadillo->setText(texto);
            bocadillo->adjustSize();

            actualizarPosicionBocadillo();
            bocadillo->show();


            timerOcultar->start(4000);
        }
    }

private:
    QLabel *labelTux;
    QPixmap pixmap;
    QPoint posicionPrevia;
    QLabel *bocadillo;
    QStringList mensajes;
    QTimer *timerMovimiento;
    QTimer *timerCaminata;
    int pasosRestantes = 0;
    QPoint direccionPaso;
    QTimer *timerMensajes;
    QTimer *timerOcultar;
    QTimer *timerAnimacion;
    int pasoAnimacion = 0;
    void actualizarPosicionBocadillo() {
        QPoint posTux = this->pos();
        bocadillo->move(posTux.x() + 90, posTux.y() + margenSalto - 25);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    VentanaFlotante ventana;
    ventana.show();
    return app.exec();
}

#include "main.moc"
