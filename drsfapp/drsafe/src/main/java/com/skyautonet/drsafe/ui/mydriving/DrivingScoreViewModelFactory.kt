import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.skyautonet.drsafe.ui.mydriving.DrivingScoreRepository

class DrivingScoreViewModelFactory(private val repository: DrivingScoreRepository) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(DrivingScoreViewModel::class.java)) {
            return DrivingScoreViewModel(repository) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}
