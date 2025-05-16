import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.skyautonet.drsafe.ui.mydriving.DrivingScoreRepository

class DrivingScoreViewModel(private val repository: DrivingScoreRepository) : ViewModel() {

    private val _drivingEvents = MutableLiveData<List<DrivingEvent>>()
    val drivingEvents: LiveData<List<DrivingEvent>> get() = _drivingEvents

    init {
        loadDrivingEvents()
    }

    private fun loadDrivingEvents() {
        val events = repository.readRecentCsvEvents()
        _drivingEvents.postValue(events)
    }
}

data class DrivingEvent(
    val title: String,
    val description: String,
    val timestamp: String,
    val startLocation: LatLng,
    val endLocation: LatLng,
    val route: List<LatLng>
)

data class LatLng(val latitude: Double, val longitude: Double)
